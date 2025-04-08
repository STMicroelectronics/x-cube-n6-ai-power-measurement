# /*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2024 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/


import threading
import time

import serial

import wrapper
import numpy as np  
# -----------------------------------------------------------------------------
# STLINK PWR Section
# -----------------------------------------------------------------------------
class StlinkPwrCaptureConfig(object):
    """docstring for StlinkPwrCaptureConfig"""
    def __init__(self, volt, freq, duration, efficiency):
        super(StlinkPwrCaptureConfig, self).__init__()
        self.volt = volt
        self.freq = freq
        self.duration = duration
        self.efficiency = efficiency

    def volt_in_mv(self):
        return int(self.volt * 1000)

    def acq_freq_in_hz(self):
        return self.freq

    def sample_nb(self):
        return int(self.freq * self.duration)


class StlinkPwrCapture(threading.Thread):
    """docstring for StlinkPwrCapture"""
    def __init__(self, serial_name, name, config):
        super(StlinkPwrCapture, self).__init__()
        self.serial_name = serial_name
        self.name = name
        self.config = config
        self.stlinkpwr = wrapper.StLinkPwrWrapper(serial_name)
        self.data = []

    def setup(self):
        self.stlinkpwr.setup(self.config.volt_in_mv(),
                             self.config.acq_freq_in_hz())

    def run(self):
        self.data = self.stlinkpwr.capture(self.config.sample_nb())
        # convert in Watts
        self.data = [[I * self.config.volt * self.config.efficiency] for I in self.data]

    def wait(self):
        self.join()
        print(f"STLINKpwr {self.name} data received")
        return self.data

    def get_name(self):
        return [self.name]

    def get_data(self):
        return self.data

    def on(self):
        self.stlinkpwr.set_volt(self.config.volt_in_mv())
        self.stlinkpwr.enable_power()

    def off(self):
        self.stlinkpwr.disable_power()

# -----------------------------------------------------------------------------
# STLINK VIRTUAL COM Section
# -----------------------------------------------------------------------------
class StlinkComPortConfig(object):
    """docstring for StlinkPwrCaptureConfig"""
    def __init__(self, freq, dur):
        super(StlinkComPortConfig, self).__init__()
        self.freq = freq
        self.dur = dur
        self.samples_nb = int(freq * dur)

class StlinkComPort(threading.Thread):
    # Markers
    start_marker = '[SLP_SOL]'
    end_marker = '[SLP_EOL]'
    end_of_transmit = "END_OF_LOG"

    """Class for handling virtual COM port communication."""
    def __init__(self, config, fields_name, port, baud_rate=115200, timeout=1):
        super(StlinkComPort, self).__init__()
        self.port = port
        self.baud_rate = baud_rate
        self.timeout = timeout
        self.serial_connection = None
        self.running = False
        self.data = []
        self.rawdata = []
        self.fields_name = fields_name
        self.config = config

    """ 
    Filters out elements from the list where the difference between the current value
    and the previous value is less than 1/sampling_freq. 
    """
    @staticmethod
    def _filter_received_data(data, step):
        filtered_data = []
        previous_value = None

        for element in data:
            current_value = element[1]
            # Check if the difference is >= step
            if previous_value is not None and (current_value - previous_value) < step:
                print(f"   | info > Filtered out sequence: {element[0]}")
            else:
                filtered_data.append(element)
            # Update the previous value to the current value for the next iteration
            previous_value = current_value              
        return filtered_data
    

    @staticmethod
    def create_output_list(datas, sampling_freq, sample_nb):
        step = 1 / sampling_freq
        datas = StlinkComPort._filter_received_data(datas, step)
        
        sequence_name = np.array([d[0] for d in datas])
        timestamps = np.array([d[1] for d in datas])
        n6_rcc_enr_register = np.array([d[2:] for d in datas], dtype=object)
        
        result = np.zeros((sample_nb, 3 + 15), dtype=object)
        result[:, 2] = ''  # Initialize the sequence name column with empty strings
        
        sequence_idx = 0
        done_for_seq = False
        for i in range(sample_nb):
            current_time = i / sampling_freq
            sequence_status = 0

            mask = (timestamps >= current_time) & (timestamps < (current_time + step))
            if np.any(mask):
                sequence_status = 1
                sequence_idx += 1
                timestamps = np.delete(timestamps, np.where(mask)[0][0])
                done_for_seq = False
            
            result[i, 0] = sequence_status
            result[i, 1] = sequence_idx
            result[i, 2] = sequence_name[sequence_idx] if sequence_idx < len(sequence_name) else "NOT_FOUND"
            if sequence_idx < len(n6_rcc_enr_register) and not done_for_seq:
                done_for_seq = True
                result[i, 3:] = n6_rcc_enr_register[sequence_idx]
            else:
                result[i, 3:] = ""
        
        return result.tolist()    

    @staticmethod
    def extract_specific_integer(data_string):
        # Split the string by ":"
        split_data = data_string.split(":")
        unit_to_prescaler = {
            "ns": 1000000000.0, # nanoseconds to seconds
            "us": 1000000.0,    # microseconds to seconds
            "ms": 1000.0        # milliseconds to seconds
        }
        pres = float(unit_to_prescaler.get(split_data[2]))
        # Extract the integer from the middle part
        if split_data[1]:
            timestamp = int(split_data[1])/pres  # Convert to integer
        else:
            timestamp = None
        
        # Prepare the result in the desired format
        split_data[1] = timestamp if timestamp is not None else 0 #add timestamp in seconds
        del split_data[2] #remove timestamp unit from list
        return split_data

    # Function to process incoming data
    @staticmethod
    def process_data(data):

        if data.startswith(StlinkComPort.start_marker) and data.endswith(StlinkComPort.end_marker):
            clean_data = data[len(StlinkComPort.start_marker):-len(StlinkComPort.end_marker)]
            return clean_data
        else:
            return None
    def setup(self):
        """Set up the serial connection."""
        try:
            self.serial_connection = serial.Serial(
                port=self.port,
                baudrate=self.baud_rate,
                timeout=self.timeout
            )
        except serial.SerialException as e:
            print(f"Could not open serial port {self.port}: {e}")
    
    def run(self):
        """Read data from the COM port and process it before appending."""
        if self.serial_connection and self.serial_connection.is_open:
            self.running = True
            buffer = ''  # Initialize a buffer to hold incoming data
            all_data = []
            while self.running:
                try:
                    if self.serial_connection.in_waiting:
                        incoming_data = self.serial_connection.read(self.serial_connection.in_waiting).decode('utf-8')
                        if incoming_data:
                            buffer += incoming_data

                            if StlinkComPort.end_of_transmit in buffer:
                                self.running = False
                                break
                            while StlinkComPort.end_marker in buffer:
                                start_index = buffer.find(StlinkComPort.start_marker)
                                end_index = buffer.find(StlinkComPort.end_marker) + len(StlinkComPort.end_marker)
                                if start_index != -1 and end_index != -1:
                                    data_to_process = buffer[start_index:end_index]
                                    buffer = buffer[end_index:]
                                    clean_data = StlinkComPort.process_data(data_to_process)
                                    if clean_data is not None:
                                        all_data.append(clean_data)
                except serial.SerialException as e:
                    print(f"Serial exception: {e}")
                    self.running = False
            self.rawdata = all_data

    def stop(self):
        """Stop reading data from the COM port."""
        self.running = False
        self.join()
    
    def wait(self):
        self.join()
        print("Timestamp received from DK")
        return self.data
    
    def get_data(self):
        """Get the data read from the COM port."""
        #process data
        self.data = StlinkComPort.create_output_list ( 
            [StlinkComPort.extract_specific_integer(data) for data in self.rawdata],
            self.config.freq,
            self.config.samples_nb)
        return self.data
    
    def get_name(self):
        return self.fields_name
    
    def write_data(self, data):
        """Write data to the COM port."""
        if self.serial_connection and self.serial_connection.is_open:
            try:
                self.serial_connection.write(data.encode('utf-8'))
            except serial.SerialException as e:
                print(f"Could not write to serial port {self.port}: {e}")

    def close(self):
        """Close the serial connection."""
        if self.serial_connection and self.serial_connection.is_open:
            self.serial_connection.close()
    
    def on(self):
        pass

    def off(self):
        pass