# /*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2024 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/


import argparse
import copy
import csv
import sys
import time
import wrapper
import serial.tools.list_ports as list_ports
import yaml
from statistics import mean
from itertools import cycle

import matplotlib.pyplot as plt
from matplotlib.lines import Line2D

from toolbox import StlinkPwrCaptureConfig, StlinkPwrCapture
from toolbox import StlinkComPortConfig, StlinkComPort

POWER_COLORS = {
    'VDDCORE': '#00B050',
    'VDDA1V8': '#7030A0',
    'VDDIO': '#39A9DC',
    'VDDA1V8_AON': '#FFC000'
}

POWER_ON_ORDER = [
    'VDDIO',
    'VDDA1V8_AON',
    'VDDA1V8',
    'VDDCORE'
]

cb_args = None

def is_stlink_pwr_ctrl(port):
    return port.description.find("ST-Link VCP-PWR Ctrl") != -1 or port.description.find("STLink Virtual COM Port PWR") != -1

def get_stlink_pwr_devices():
    ports = list_ports.comports()

    return [port for port in ports if is_stlink_pwr_ctrl(port)]

def get_all_stlinks():
    ports = list_ports.comports()
    return [port for port in ports if port.description.find("STLink Virtual COM Port") != -1] 

def get_port_name_by_serial(serial):
    ports = get_stlink_pwr_devices()
    ports = [port for port in ports if port.serial_number == serial]

    return "" if len(ports) == 0 else ports[0].device

def list_devices(args):
    ports = get_all_stlinks()
    if not ports:
        print("Unable to detect any ST-Link Power Ctrl")
    else:
        print("Detected %d ST-Link Power Ctrl device(s)" % len(ports))
        for port in ports:
            pwr_status = "(STLink PWR)" if is_stlink_pwr_ctrl(port) else ""
            print("  %s %s %s" % (port.device, port.serial_number, pwr_status))

def read_configuration(conf_filename):
    with open(conf_filename, 'r') as file:
        config = yaml.safe_load(file)

    return config

def apply_stlink_pwr_ctrl(device, freq, duration):
    config = StlinkPwrCaptureConfig(float(device['vdd']), freq, duration, float(device['efficiency']))
    dev = StlinkPwrCapture(get_port_name_by_serial(device['serial']), device['name'], config)

    return dev

def get_stlink_com_by_serial(device_serial):
    ports = get_all_stlinks()
    for port in ports:
        if port.serial_number == device_serial:
            return port.device  # Return the string directly

    return None  # or return an empty string '' if no match is found

def apply_stlink_com_ctrl(device, freq, duration):
    config = StlinkComPortConfig(freq, duration)
    StlinkComPortField = ['seq_changed','seq_idx','seq_name', 
                        'RCC_DIVENR', 'RCC_MISCENR', 'RCC_MEMENR', 
                        'RCC_AHB1ENR', 'RCC_AHB2ENR', 'RCC_AHB3ENR', 
                        'RCC_AHB4ENR', 'RCC_AHB5ENR', 'RCC_APB1LENR', 
                        'RCC_APB1HENR', 'RCC_APB2ENR', 'RCC_APB3ENR', 
                        'RCC_APB4LENR', 'RCC_APB4HENR', 'RCC_APB5ENR' ]

    dev = StlinkComPort(config, StlinkComPortField , get_stlink_com_by_serial(device['serial']), device['baud_rate'])

    return dev

def apply_configuration(config, freq, duration):
    devices = []

    for device in config['devices']:
        if device['type'] == "stlink_pwr_ctrl":
            devices.append(apply_stlink_pwr_ctrl(device, freq, duration))
        elif device['type'] == "stlink_com_ctrl":
            devices.append(apply_stlink_com_ctrl(device, freq, duration))
        else:
            assert(False)

    return devices

def get_next_power_on_device_idx(devices, power_state):
    # try first in POWER_ON_ORDER
    for name in POWER_ON_ORDER:
        for (idx, device) in enumerate(devices):
            if name == device.get_name()[0] and not power_state[idx]:
                return idx

    # turn on remaining devices
    for (idx, device) in enumerate(devices):
        if not power_state[idx]:
            return idx

    return -1

def get_power_on_order_idx(devices):
    res = []
    power_state = [False for d in devices]

    while True:
        idx = get_next_power_on_device_idx(devices, power_state)
        if idx < 0:
            break;
        power_state[idx] = True
        res.append(idx)

    return res

def power(args):
    assert(args.enabled == 'on' or args.enabled == 'off')
    config = read_configuration(args.config)
    devices = apply_configuration(config, 100000, 1.0)

    power_order = get_power_on_order_idx(devices)
    if args.enabled == 'on':
        for idx in power_order:
            devices[idx].on()
            if ('VDD' in devices[idx].get_name()[0]):
                print("%s ON" % devices[idx].get_name()[0])
            time.sleep(0.1)
    else:
        for idx in reversed(power_order):
            devices[idx].off()

    print("Power interface %s" % args.enabled)

def get_max_power_from_results(result):
    max_power = 0
    for r in result[1:-3]:
        if r['is_gpio']:
            continue
        print(r['data'])
        max_power_r = max(r['data'])
        if max_power_r > max_power:
            max_power = max_power_r

    return max_power

def filter_per_time_range(datas, start, end):
    result = []
    for d in datas:
        if d[0] < start:
            continue
        if d[0] > end:
            continue
        result.append(d)

    return result

def process_datas(names, is_gpios, rows, start, end, power_rescale):
    result = []
    for (i, is_gpio) in enumerate(is_gpios[1:]):
        if is_gpio:
            continue
        name = names[i + 1]
        power_in_mw = mean([r[i + 1] for r in rows]) * power_rescale
        energy_in_uj = power_in_mw * (end - start) * 1000
        result.append((name, power_in_mw, energy_in_uj))

    return result

def update_text(text, names, is_gpios, rows, ax_range, power_rescale):
    datas = filter_per_time_range(rows, ax_range[0], ax_range[1])
    name_power_info = process_datas(names, is_gpios, datas, ax_range[0], ax_range[1], power_rescale)

    power_in_mw = sum([info[1] for info in name_power_info])
    energy_in_uj = sum([info[2] for info in name_power_info])
    duration_in_ms = (ax_range[1] - ax_range[0]) * 1000

    textstr = "Total :\n %.2f mW\n %.2f uJ\n %.2f ms\n" % (power_in_mw ,energy_in_uj, duration_in_ms)
    for info in name_power_info:
        textstr += "%s :\n %.2f mW\n %.2f uJ\n" % info

    text.set_text(textstr)

def on_ylims_change(evt):
    update_text(cb_args[3], cb_args[0], cb_args[1], cb_args[2], evt.get_xlim(), cb_args[4])

def capture(args):
    config = read_configuration(args.config)
    devices = apply_configuration(config, args.pwr_freq, args.pwr_duration)

    # setup devices
    for device in devices:
        device.setup()
    # start devices
    for device in devices:
        device.start()
    print("CAPTURING : press wake-up button")
    # wait end of capture
    for device in devices:
        device.wait()
    print("CAPTURE DONE")

    # retriewe datas
    result = []
    for device in devices:
        datas = device.get_data()
        names = device.get_name()
        for (i, name) in enumerate(names):
            data = [d[i] for d in datas]
            result.append({
                'name': name,
                'data': data,
                'is_gpio': len(names) == 3
                })

    result.insert(0, {
        'name': "time",
        'data': [v / args.pwr_freq for v in range(len(result[0]['data']))],
        'is_gpio': False
    })
    names = [r['name'] for r in result]
    is_gpios = [r['is_gpio'] for r in result]
    rows = []
    for i in range(len(result[0]['data'])):
        rows.append([r['data'][i] for r in result])

    if args.write_csv:
        # write csv
        with open(args.write_csv, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(names)
            writer.writerows(rows)
    else:
        assert(0) # data to be written to a csv file

def display_get_linestyle(names):
    return ['-' if name in POWER_COLORS else 'solid' for name in names]

def display_get_colors(names):
    cycol = cycle('kbgrcm')

    return [POWER_COLORS[name] if name in POWER_COLORS else next(cycol) for name in names]

def display_get_labels(names):
    return [name if name in POWER_COLORS else None for name in names]

def display_get_is_gpios(names):
    return [False if name in POWER_COLORS else True for name in names]

def display_get_max_values(rows):
    res = []
    for i in range(len(rows[0])):
        res.append(max([r[i] for r in rows]))

    return res

def display_get_max_value(max_values, is_gpios):
    return max([m if not is_gpios[i] else 0.0 for i, m in enumerate(max_values)])

def display_rescale_rows(rows, is_gpios, max_power_value):
    res = []

    for r in rows:
        res.append([d * max_power_value * 1000 if is_gpios[i] else d * 1000 for i, d in enumerate(r)])

    return res

def display(args):
    global cb_args

    with open(args.read_csv, newline='') as f:
        reader = csv.reader(f)
        rows = list(reader)
        names = rows[0]
        assert(names[0] == 'time')
        full_names = names
        names = names[1:-17] #RCC regs not considered
        rows = rows[1:]
        x = [float(r[0])for r in rows]
        rows = [[float(d) for d in r[1:-17]] for r in rows]
        linestyle = display_get_linestyle(names)
        colors = display_get_colors(names)
        labels = display_get_labels(names)
        is_gpios = display_get_is_gpios(names)
        max_values = display_get_max_values(rows)
        max_power_value = display_get_max_value(max_values, is_gpios)
        rows = display_rescale_rows(rows, is_gpios, max_power_value)

        full_rows = copy.deepcopy(rows)
        for i, r in enumerate(full_rows):
            r.insert(0, x[i])
        full_is_gpios = is_gpios
        full_is_gpios.insert(0, False)
        ax = plt.gca()
        ax.callbacks.connect('ylim_changed', on_ylims_change)
        props = dict(boxstyle='round', facecolor='wheat', alpha=0.5)
        text = ax.text(0.95, 0.95, "", transform=ax.transAxes, fontsize=10, va='top', ha='right', ma='left', bbox=props)
        cb_args = (full_names, is_gpios, full_rows, text, 1.0)

        for i, c in enumerate(colors):
            if names[i] == 'seq_changed':
                # Step 2: Identify the positions where the value is 1
                column_values = [r[i] for r in rows]
                max_value = int(max(column_values))
                positions = [j for j, value in enumerate([r[i] for r in rows]) if value >= max_value]
                # Step 3: Create the plot using plt.plot
                for pos in positions:
                    plt.plot([x[pos], x[pos]], [0, max_value], 'r-')  # 'r-' means red line
            else:
                plt.plot(x, [r[i] for r in rows], color=c, label=labels[i], linestyle=linestyle[i])
        plt.ylabel("P in mW")
        plt.xlabel("Time in seconds")
        plt.legend(loc='upper left')
        plt.grid()
        plt.show()

def parse_args():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(help='sub-command help', required=False)

    parser_power = subparsers.add_parser('list', help='list available ST-Link Power Ctrl devices')
    parser_power.set_defaults(func=list_devices)

    parser_power = subparsers.add_parser('power', help='power help')
    parser_power.add_argument('-c', '--config', help='yaml configuration file', required=True)
    parser_power.add_argument('enabled', help='on or off')
    parser_power.set_defaults(func=power)

    parser_power = subparsers.add_parser('capture', help='capture help')
    parser_power.add_argument('-c', '--config', help='yaml configuration file', required=True)
    parser_power.add_argument('-f', '--pwr_freq', default=100000, help='capture frequency in hz', type=int)
    parser_power.add_argument('-d', '--pwr_duration', default=1.0, help='capture duration in seconds', type=float)
    parser_power.add_argument('-w', '--write_csv', help='csv file to write', required=True)
    parser_power.set_defaults(func=capture)

    parser_power = subparsers.add_parser('display', help='display help')
    parser_power.add_argument('-r', '--read_csv', help='csv file to read')
    parser_power.set_defaults(func=display)

    return parser.parse_args()

def main(parser):
    if 'func' in parser:
        parser.func(parser)
    else:
        parser.print_help()

if __name__ == '__main__':
    main(parse_args())
