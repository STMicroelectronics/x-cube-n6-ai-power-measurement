# /*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2024 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/


import argparse
import csv
import sys
from statistics import mean


POWER_NAME = [
"VDDCORE",
"VDDA1V8",
"VDDIO",
"VDDA1V8_AON"
]

def filter_sequence_samples(rows):
  in_sequence = False
  prev_pa3_value = 0
  result = []

  # only report first sequence
  for row in rows:
    if 'PA3' in row:
      if float(row['PA3']) == 1 and prev_pa3_value == 0:
        in_sequence = True
      if float(row['PA3']) == 0 and prev_pa3_value == 1:
        break
      if in_sequence:
        result.append(row)
      prev_pa3_value = float(row['PA3'])
    elif 'seq_name' in row:
      if row['seq_name'] != 'NOT_FOUND':
        result.append(row)
    else:
      assert(0)

  return result

def get_time_list(rows):
  return [float(r['time']) for r in rows]

def get_power_list(rows, name):
  return [float(r[name]) for r in rows]


def filter_sequence_samples_per_index(rows, seq_idx) :
  result = []

  for r in rows:
    current = int(r['seq_idx']) 
    if current == seq_idx:
      result.append(r)

  return result

def filter_sequence_samples_per_state(rows, seq_idx):
  rows = filter_sequence_samples_per_index(rows, seq_idx)

  return rows

def get_available_power_name(rows):
  r = rows[0]
  res = []
  for name in POWER_NAME:
    if name in r:
      res.append(name)

  return res

# Define the registers and their bits
registers = {
    "RCC_DIVENR": {
        "bits": ["Res."] * 12 + ["IC20EN", "IC19EN", "IC18EN", "IC17EN", "IC16EN", "IC15EN", "IC14EN", "IC13EN", "IC12EN", "IC11EN", "IC10EN", "IC9EN", "IC8EN", "IC7EN", "IC6EN", "IC5EN", "IC4EN", "IC3EN", "IC2EN", "IC1EN"]
    },
    "RCC_MISCENR": {
        "bits": ["Res."] * 25 + ["PEREN", "Res.", "Res.", "XSPIPHYCOMPEN", "MCO2EN", "MCO1EN", "DBGEN"]
    },
    "RCC_MEMENR": {
        "bits": ["Res."] * 19 + ["BOOTROMEN", "VENCRAMEN", "CACHEAXIRAMEN", "FLEXRAMEN", "AXISRAM2EN", "AXISRAM1EN", "BKPSRAMEN", "AHBSRAM2EN", "AHBSRAM1EN", "AXISRAM6EN", "AXISRAM5EN", "AXISRAM4EN", "AXISRAM3EN"]
    },
    "RCC_AHB1ENR": {
        "bits": ["Res."] * 26 + ["ADC12EN", "GPDMA1EN", "Res.", "Res.", "Res.", "Res."]
    },
    "RCC_AHB2ENR": {
        "bits": ["Res."] * 14 + ["ADF1EN", "MDF1EN", "Res.", "Res.", "Res.", "RAMCFGEN", "Res.", "Res.", "Res.", "Res.", "Res.", "Res.", "Res.", "Res.", "Res.", "Res.", "Res.", "Res."]
    },
    "RCC_AHB3ENR": {
        "bits": ["Res."] * 17 + ["RISAFEN", "Res.", "Res.", "Res.", "IACEN", "RIFSCEN", "PKAEN", "Res.", "Res.", "Res.", "SAESEN", "Res.", "CRYPEN", "HASHEN", "RNGEN"]
    },
    "RCC_AHB4ENR": {
        "bits": ["Res."] * 12 + ["CRCEN", "PWREN", "Res.", "GPIOQEN", "GPIOPEN", "GPIOOEN", "GPIONEN", "Res.", "Res.", "Res.", "Res.", "Res.", "GPIOHEN", "GPIOGEN", "GPIOFEN", "GPIOEEN", "GPIODEN", "GPIOCEN", "GPIOBEN", "GPIOAEN"]
    },
    "RCC_AHB5ENR": {
        "bits": ["NPUEN", "CACHEAXIEN", "OTG2EN", "OTGPHY2EN", "OTGPHY1EN", "OTG1EN", "ETH1EN", "ETH1RXEN", "ETH1TXEN", "ETH1MACEN", "Res.", "GPU2DEN", "GFXMMUEN", "MCE4EN", "XSPI3EN", "MCE3EN", "MCE2EN", "MCE1EN", "XSPIMEN", "XSPI2EN", "Res.", "Res.", "Res.", "SDMMC1EN", "SDMMC2EN", "PSSIEN", "XSPI1EN", "FMCEN", "JPEGEN", "Res.", "DMA2DEN", "HPDMA1EN"]
    },
    "RCC_APB1LENR": {
        "bits": ["UART8EN", "UART7EN", "Res.", "Res.", "Res.", "Res.", "I3C2EN", "I3C1EN", "I2C3EN", "I2C2EN", "I2C1EN", "UART5EN", "UART4EN", "USART3EN", "USART2EN", "SPDIFRX1EN", "SPI3EN", "SPI2EN", "TIM11EN", "TIM10EN", "WWDGEN", "Res.", "LPTIM1EN", "TIM14EN", "TIM13EN", "TIM12EN", "TIM7EN", "TIM6EN", "TIM5EN", "TIM4EN", "TIM3EN", "TIM2EN"]
    },
    "RCC_APB1HENR": {
        "bits": ["Res."] * 13 + ["UCPD1EN", "Res.", "Res.", "Res.", "Res.", "Res.", "Res.", "Res.", "Res.", "Res.", "FDCANEN", "Res.", "Res.", "MDIOSEN", "Res.", "Res.", "Res.", "Res.", "Res."]
    },
    "RCC_APB2ENR": {
        "bits": ["Res."] * 9 + ["SAI2EN", "SAI1EN", "SPI5EN", "TIM9EN", "TIM17EN", "TIM16EN", "TIM15EN", "TIM18EN", "Res.", "SPI4EN", "SPI1EN", "Res.", "Res.", "Res.", "Res.", "USART10EN", "UART9EN", "USART6EN", "USART1EN", "Res.", "Res.", "TIM8EN", "TIM1EN"]
    },
    "RCC_APB3ENR": {
        "bits": ["Res."] * 29 + ["DFTEN", "Res.", "Res."]
    },
    "RCC_APB4LENR": {
        "bits": ["Res."] * 14 + ["RTCAPBEN", "RTCEN", "VREFBUFEN", "Res.", "Res.", "LPTIM5EN", "LPTIM4EN", "LPTIM3EN", "LPTIM2EN", "Res.", "I2C4EN", "Res.", "SPI6EN", "Res.", "LPUART1EN", "HDPEN", "Res.", "Res."]
    },
    "RCC_APB4HENR": {
        "bits": ["Res."] * 29 + ["DTSEN", "BSECEN", "SYSCFGEN"]
    },
    "RCC_APB5ENR": {
        "bits": ["Res."] * 25 + ["CSIEN", "VENCEN", "GFXTIMEN", "Res.", "DCMIPPEN", "LTDCEN", "Res."]
    }
}

def _process_rcc_regs(register_name, value):
    if register_name not in registers:
        return "Register not found."
    register = registers[register_name]
    bits = register["bits"]
    binary_value = f"{value:032b}"[::]  # Convert to 32-bit binary  
    # List comprehension to remove "EN" only if it is at the end of the string
    activated_ips = [
    bits[i][:-2] if bits[i].endswith("EN") else bits[i]
    for i in range(len(bits))
    if bits[i] != "Res." and binary_value[i] == '1' ]
    return activated_ips

def get_clocked_ips(rows):
    # Initialize an empty dictionary to store the results
    rcc_enr_regs = {}
    # Iterate over each key-value pair in the input dictionary
    for key, value in rows.items():
        # Check if the key starts with 'RCC_'
        if key.startswith('RCC_'):
            # Extract the integer value from the string (after the '=' sign)
            int_value = int(value.split('=')[1])
            # Add the key and integer value to the result dictionary
            ips_ = _process_rcc_regs(key, int_value)
            #if ips_ :
            result = ' | '.join(ips_)
            rcc_enr_regs[key] = result

    return rcc_enr_regs

def get_power_for_state(rows, seq_idx):
  res = {}
  if not rows:
    return
  activ_ips = get_clocked_ips(rows[0])
  power_name = get_available_power_name(rows)
  state_time = get_time_list(rows)[-1] - get_time_list(rows)[0]
  powers = []
  for name in power_name:
    mean_power = mean(get_power_list(rows, name))
    min_power = min(get_power_list(rows, name))
    max_power = max(get_power_list(rows, name))
    energy = mean_power * state_time
    powers.append((name, mean_power, energy, state_time, min_power, max_power))
  r = rows[0]
  if r['seq_name'] == 'NOT_FOUND': # filter out unknown sequence
    return
  res['seq_idx'] = seq_idx
  res['seq_name'] = r['seq_name']
  res['datas'] = powers
  res['activ_ips'] = activ_ips
  return res

def get_power_per_state(rows):
  res = []
  for i in range(100):
    rows_state = filter_sequence_samples_per_state(rows, i)
    row_res = get_power_for_state(rows_state, i)
    if row_res:
      res.append(row_res)

  return res

def get_total_power(data):
  return sum([d[1] for d in data])

def get_max_power(data):
    return max([d[5] for d in data])

def get_min_power(data):
    return min([d[4] for d in data])

def get_total_energy(data):
  return sum([d[2] for d in data])

def display_raw_state(datas_state, is_verbose):
  total_power = get_total_power(datas_state['datas'])
  total_energy = get_total_energy(datas_state['datas'])
  elapsed_time = datas_state['datas'][0][3]

  print("%2d: %32.1f uJ in %8.1f ms (%7.2f mW)" % (datas_state['seq_idx'],
                                                  total_energy * 1000000,
                                                  elapsed_time * 1000,
                                                  total_power * 1000))
  if is_verbose:
    for data in datas_state['datas']:
      print("   %-24s %8.1f uJ (%7.2f mW)" % (data[0], data[2] * 1000000,
                                              data[1] * 1000))

def display_raw(datas, is_verbose):
  for data in datas:
    display_raw_state(data, is_verbose)

def display_cooked_state(datas_state, is_verbose, is_clocked):
    total_power = get_total_power(datas_state['datas'])
    total_energy = get_total_energy(datas_state['datas'])
    elapsed_time = datas_state['datas'][0][3]
    idx = datas_state['seq_idx']
    name = datas_state['seq_name']

    print('\n'
        f"step {idx:2} : {total_energy * 1000000:16.2f} uJ"
        f" in {elapsed_time * 1000:8.2f} ms"
        f" for {name:30s}"
        f" ({total_power * 1000:7.2f} mW)"
    )
    if is_verbose:
      print(f"   Measured pwr")
      for i, data in enumerate(datas_state['datas']):
          if i == len(datas_state['datas']) - 1:
              print("       └── %-9s %8.1f uJ (avg %7.2f mW, min %7.2f mW, max %7.2f mW)" % (
                  data[0], data[2] * 1000000, data[1] * 1000, data[4] * 1000, data[5] * 1000))
          else:
              print("       ├── %-9s %8.1f uJ (avg %7.2f mW, min %7.2f mW, max %7.2f mW)" % (
                  data[0], data[2] * 1000000, data[1] * 1000, data[4] * 1000, data[5] * 1000))        
    if is_clocked:
      print(f"   Clocked IPs")
      keys = list(datas_state['activ_ips'].keys())
      for i, key in enumerate(keys):
          if i == len(keys) - 1:
              print(f"       └── {key}: {datas_state['activ_ips'][key]}")
          else:
              print(f"       ├── {key}: {datas_state['activ_ips'][key]}")
      print("")
    return (total_energy * 1000000, elapsed_time * 1000)


def get_verbose_info_for_name(datas, name):
  energy_t = 0

  for data in datas:
    idx = data['seq_idx']
    for info in data['datas']:
      if info[0] != name:
        continue
      energy_t += info[2]

  return (name, energy_t)

def display_cooked(datas, is_verbose, is_clocked):
  energy = 0
  duration = 0
  energy_t = 0
  duration_t = 0

  print("--------------------------------------------------------------------------------------------")
  for data in datas:
    (e, d) = display_cooked_state(data, is_verbose, is_clocked)
    energy += e
    duration += d
    energy_t += e
    duration_t += d

  verbose_info = []
  if is_verbose:
    power_name = [d[0] for d in datas[0]['datas']]
    for name in power_name:
      verbose_info.append(get_verbose_info_for_name(datas, name))

  print("--------------------------------------------------------------------------------------------")
  print(' ' *13 + f"{energy_t:8.1f} uJ in {duration_t:8.1f} ms for all steps ({energy_t/duration_t:8.1f}mW ) ")
  if is_verbose:
    for info in verbose_info:
      print("   %-9s %8.1f uJ" % (info[0], info[1] * 1000000))

def main(args):
  with open(args.csv_filename, newline='') as f:
    reader = csv.DictReader(f)
    rows = list(reader)
    rows = filter_sequence_samples(rows)
    full_sequence_time = get_time_list(rows)[-1] - get_time_list(rows)[0]
    res = get_power_per_state(rows)
    if args.raw:
      display_raw(res, args.verbose)
    else:
      display_cooked(res, args.verbose, args.clocked_ip)

def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('csv_filename')
    parser.add_argument('-r', '--raw', action='store_true')
    parser.add_argument('-v', '--verbose', action='store_true', help='Increase output verbosity')
    parser.add_argument('-c', '--clocked_ip', action='store_true', help='display clock IPs tree')

    args = parser.parse_args()
    return args
if __name__ == '__main__':
  main(parse_args())
