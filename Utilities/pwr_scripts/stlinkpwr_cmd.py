# /*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2024 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/


import logging
import re
import statistics
import time
from collections import deque

import serial

from list_port_coms import COMPorts

# create a logger with a unique name
logger = logging.getLogger(__name__)

# set the logging level for the logger
logger.setLevel(logging.FATAL)

# create a stream handler to output log messages to the console
handler = logging.StreamHandler()
handler.setLevel(logging.FATAL)

# create a formatter to format log messages
formatter = logging.Formatter("%(levelname)s: %(message)s")
handler.setFormatter(formatter)

# add the handler to the logger
logger.addHandler(handler)

# Serial Commands of STLinkPWR device
# every command is a dictionary
# {
#     "command_base": " the command to be sent to the device for example 'freq' ",
#     "expected_response": " the header of the expected response when the command was successful example 'ack freq' ",
#     "delimiter": "the end caraters of the replu the stlinkpwr send these chars '\r\n\r\n'"
# }
# see below and the manual if you want to add other commands

COMMAND_INFO = dict(set_freq={
    "command_base": "freq",
    "expected_response": "ack freq",
    "delimiter": "\r\n\r\n"
}, set_format={
    "command_base": "format",
    "expected_response": "ack format",
    "delimiter": "\r\n\r\n"
}, set_acqtime={
    "command_base": "acqtime",
    "expected_response": "ack acqtime",
    "delimiter": "\r\n\r\n"
}, set_volt={
    "command_base": "volt",
    "expected_response": "ack volt",
    "delimiter": "\r\n\r\n"
}, get_id={
    "command_base": "powershield",
    "expected_response": "ack STLINK-V3PWR",
    "delimiter": "\r\n\r\n"
}, get_vr={
    "command_base": "version",
    "expected_response": "ack version",
    "delimiter": "\r\n\r\n"
}, set_power_monitor={
    "command_base": "power_monitor",
    "expected_response": "ack power_monitor",
    "delimiter": "\r\n\r\n"
}, get_temp_degc={
    "command_base": "temp degc",
    "expected_response": "ack temp degc",
    "delimiter": "\r\n\r\n"
}, get_temp_degf={
    "command_base": "temp degf",
    "expected_response": "ack temp degf",
    "delimiter": "\r\n\r\n"
}, get_temp_pcb={
    "command_base": "temp refresh pcb",
    "expected_response": "ack temp refresh pcb",
    "delimiter": "\r\n\r\n"
}, set_power={
    "command_base": "pwr",
    "expected_response": "ack pwr",
    "delimiter": "\r\n\r\n"
})

# the constant of the stlinkpwr port description
STLINKPWR_PORTDESC = "STMicroelectronics STLink Virtual COM Port PWR"

# the constant of the fixed metadata frame sizes
STATIC_FRAME_SIZES = {0xf3: 9, 0xf4: 4, 0xf5: 8, 0xf6: 4, 0xf7: 6, 0xf8: 6, 0xf9: 5, 0xfa: 4, 0xfb: 4}


class STLINKPWRCMD:
    """
    this class will control the STLinkPWR and communicate with it to obtain measurements.
    """
    meansamples = -1.0  # the mean of current samples
    maxsamples = -1.0  # the max of current samples
    minsamples = -1.0  # the min of current samples
    ptpsamples = -1.0  # the peek to peek of current samples
    rmsstdevsamples = -1.0  # the RMS Standard deviation of current samples
    temppcb = -100.0  # the temprature of the stlinkPWR pcb

    @staticmethod
    def _validate_port(port):
        """ method to validate the port argument by verify that the port is for a stlinkpwr device  """
        if isinstance(port, str):
            desc = COMPorts.get_description_by_device(port)
            if STLINKPWR_PORTDESC in desc:
                return port
            raise ValueError(f"Invalid port name : {port}. STlink PWR device not connected in this port")
        raise ValueError(
            f"Invalid port name format: {port}. Port name should be a string")

    @staticmethod
    def _validate_voltage(voltage):
        """ method to validate the Voltage value must be in the range [1600, 3600] """
        if isinstance(voltage, int) and 1600 <= voltage <= 3600:
            return voltage
        raise ValueError(f"Invalid voltage value: {voltage}. Voltage value must be in the range [1600, 3600] (mV).")

    @staticmethod
    def _validate_frequency(frequency):
        """
        method to validate the "frequency" value must be one of the following of VALID_FREQUENCIES list
         if is an integer or a string in this format "xxxk" if the value is >= 1000 (Hz)
             and their numerical version is part of the VALIS_FREQUENCES list """
        VALID_FREQUENCIES = [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000]
        if isinstance(frequency, int) and frequency in VALID_FREQUENCIES:
            return frequency
        elif isinstance(frequency, str) and frequency.endswith("k"):
            freq = int(frequency[:-1])
            if freq >= 1000:
                return freq
        raise ValueError(
            f"Invalid frequency value: {frequency}. Frequency must be one of the following values: {VALID_FREQUENCIES} (Hz) or 'xxxk' if the value is >= 1000 (Hz).")

    @staticmethod
    def _validate_acq_time(acq_time):
        """
        method to validate the acq_time value must be in the range [1600, 3300] if it's integer
        or must be 'inf' if it's a string
        """
        if acq_time == 0 or acq_time == 'inf':
            return acq_time
        elif isinstance(acq_time, int) and 100 <= acq_time <= 100000000:
            return acq_time
        raise ValueError(
            f"Invalid acquisition time value: {acq_time}. Acquisition time must be a numerical value in the range [100us; 100s] or '0' for infinite duration, or 'inf' for infinite duration.")

    def _conv_acqtime_str(self):
        """
        method to validate the acq_time value to the expected argument value for the device commands
        """
        if self.AcqTime == 0 or self.AcqTime == "inf":
            return str(self.AcqTime)
        if (self.AcqTime % 1000000) == 0:
            return str(self.AcqTime // 1000000) + "s"
        elif (self.AcqTime % 1000) == 0:
            return str(self.AcqTime // 1000) + "m"
        else:
            return str(self.AcqTime) + "u"

    def _validate_format(self, formatacq):
        """
            method to validate the format value musbt be "ascii_dec" or "bin_hex" switch the config use cases
        """
        if formatacq == "ascii_dec" and (self.frequency > 10000 or self.frequency != 0 or self.frequency != "inf"):
            raise ValueError("format 'ascii_dec' can only be used with frequency <= 10kHz")
        elif formatacq != "bin_hex" and formatacq != "ascii_dec":
            raise ValueError("Invalid format, only 'bin_hex' or 'ascii_dec' are accepted")
        return formatacq

    def _ask(self, command_type, *command_args):
        """
        Sends a command over a serial port, receives a reply until a delimiter is encountered,
        checks for a specific response, and returns a list of the other strings in the reply, without the expected response string,
        in addition to the boolean indicating whether the expected response was found
        """
        # Extract the command_base, expected response, and delimiter from the constant dictionary
        command_info = COMMAND_INFO[command_type]
        command_base = command_info['command_base']
        expected_response = command_info['expected_response']
        delimiter = command_info['delimiter']
        # Format the command with the given arguments
        if command_args:
            formatted_command = command_base + " " + " ".join(command_args)
        else:
            formatted_command = command_base
        logger.debug(f"cmd :[{formatted_command}]")
        # Reset the input and output buffer and flush the serial port
        self.vcp.timeout = 1
        self.vcp.reset_input_buffer()
        self.vcp.reset_output_buffer()
        self.vcp.flush()
        # Send the command over the serial port
        self.vcp.write(f"{formatted_command}\r\n".encode())
        # Receive the reply
        reply = self.vcp.read_until(delimiter.encode()).decode()
        # Reset the input and output buffer and flush the serial port
        self.vcp.reset_input_buffer()
        self.vcp.reset_output_buffer()
        self.vcp.flush()
        self.vcp.timeout = 0
        # Check if the expected response is in the reply
        logger.debug(f"reply :[{reply}]")
        if expected_response in reply:
            # Extract the other strings from the reply
            other_strings = [s for s in reply.split() if s != expected_response]
            return other_strings, True
        else:
            return reply, False

    def __init__(self, com: str):
        """
            Constructor for the STLINKPWRCMD class

            Parameters
            ----------
            com: serial COM port name of the stlinkpwr device.
        """
        self.id = None
        self.version = None
        self.format = None
        self.frequency = None
        self.AcqTime = None
        self.voltage = None
        self.bufferraw = []
        self.samples = deque()
        self.metadatas = deque()
        self.com = self._validate_port(com)
        try:
            self.vcp = serial.Serial(port=self.com,
                                     baudrate=100000,
                                     parity=serial.PARITY_NONE,
                                     stopbits=serial.STOPBITS_ONE,
                                     bytesize=serial.EIGHTBITS,
                                     timeout=0,
                                     xonxoff=False,
                                     rtscts=False,
                                     dsrdtr=False,
                                     inter_byte_timeout=None)
            responses, status = self._ask("get_id")
            if status:
                self.id = responses[2]
                responses, status = self._ask("get_vr")
                if status:
                    self.version = responses[3]
                    responses, status = self._ask("set_power_monitor")
                    if status:
                        logger.debug("The device has been successfully configured")
                    else:
                        raise ValueError(
                            f"[Error Cmd Device] setup: power monitor cmd. please check your device firmware verison")
                else:
                    raise ValueError(f"Error Com with StlinkPwr : {self.com}. please Replug the STlink PWR device")
            else:
                raise ValueError(f"Error Com with StlinkPwr : {self.com}. please Replug the STlink PWR device")
        except:
            raise ValueError(f"Error Com with StlinkPwr : {self.com}. please Replug the STlink PWR device")

    def setup(self, volt=3300, formatacq="bin_hex", acqtime="inf"):
        try:
            self.voltage = self._validate_voltage(volt)
            self.AcqTime = self._validate_acq_time(acqtime)
            self.format = self._validate_format(formatacq)

            responses, status = self._ask("set_volt", str(self.voltage) + "m")
            if status:
                time.sleep(1)
                responses, status = self._ask("set_format", self.format)
                if status:
                    responses, status = self._ask("set_acqtime", self._conv_acqtime_str())
                    if status:
                        logger.debug("The acquisition has been successfully configured")
                        return True
                    else:
                        raise ValueError(
                            f"[Error configuration settings] AcqTime: {self.AcqTime}. please check your configuration setting for this parameter")
                else:
                    raise ValueError(
                        f"[Error configuration settings] Format: {self.format}. please check your configuration setting for this parameter")
            else:
                raise ValueError(
                    f"[Error configuration settings] Voltage: {self.voltage}. please check your configuration settings for this acquisition")

        except ValueError as ve:
            print(ve)

    def enable_power(self):
        """ methode to enable the voltage output 'POWER ON' """
        try:
            responses, status = self._ask("set_power", "on")
            if not status or responses[2] != "on":
                raise ValueError(
                    f"[Error configuration settings] Power: {self.frequency}. please check your configuration setting for this parameter")

        except ValueError as ve:
            print(ve)

    def disable_power(self):
        """ methode to disable the voltage output 'POWER OFF' """
        try:
            responses, status = self._ask("set_power", "off")
            if not status or responses[2] != "off":
                raise ValueError(
                    f"[Error configuration settings] Power: {self.frequency}. please check your configuration setting for this parameter")

        except ValueError as ve:
            print(ve)

    def init_acq(self, freq=100000):
        """ methode to initialize the acquisition by setting the frequency of sampling  """
        try:
            self.frequency = self._validate_frequency(freq)

            responses, status = self._ask("set_freq", str(self.frequency))
            if not status:
                raise ValueError(
                    f"[Error configuration settings] Frequence: {self.frequency}. please check your configuration setting for this parameter")

        except ValueError as ve:
            print(ve)

    def _readline(self, timeout, delimiter):
        """
        Reads data from the connected device using a specified delimiter and timeout.

        Parameters:
        - timeout (float): The maximum time in seconds to wait for the delimiter to be received.
        - delimiter (bytes): The delimiter to use for separating the received data into lines.

        Returns:
        - buff (bytes): The received data, including the delimiter.

        Raises:
        - None

        """
        # Get the current time
        start_time = time.time()
        # Initialize the buffer for storing the received data
        buff = b''
        # Keep reading until the timeout is reached
        while (time.time() - start_time) < timeout:
            # Read one byte from the connected device
            buff += self.vcp.read(1)
            # Check if the delimiter has been received
            if buff.endswith(delimiter):
                return buff
        # Return the buffer if the timeout is reached
        return buff

    def _read_stream(self, nbrsamples=1000):
        """
        Reads data from the connected device as a stream, with the specified number of samples.

        Parameters:
        - nbrsamples (int, optional): The number of samples to read. Default is 1000.

        Returns:
        - None

        Raises:
        - ValueError: If the communication with the connected device fails.

        """
        try:
            # Clear the buffer for storing the received data
            self.bufferraw.clear()
            # Flush and reset the input and output buffers of the connected device
            self.vcp.flush()
            self.vcp.reset_input_buffer()
            self.vcp.reset_output_buffer()
            # Calculate the exact time to read the number of samples
            neededsamples = (nbrsamples + 10) * 2
            exact_time = (nbrsamples + 10) / self.frequency
            # Add 10% margin to the exact time
            timeout = exact_time * 1.1
            # Debug log
            logger.debug("==> START reading ")
            # Send the "start" command to the connected device
            self.vcp.write(b'start\r\n')
            # Read the prompt from the connected device
            prompt = self._readline(2, b'\x0d\x0a\x0d\x0a')
            # Get the start time
            start = time.time()
            # Keep reading until the specified number of samples is reached
            while len(self.bufferraw) < neededsamples:
                # Check if the timeout has been reached
                #if (time.time() - start) >= timeout:
                #    raise ValueError(f"Error Com with XX : {self.com}. please check the XX device")
                # Read any available bytes from the input buffer of the connected device
                bytes_read = self.vcp.read(self.vcp.in_waiting)
                # Check if bytes are not empty
                if bytes_read:
                    # Append the received bytes to the buffer
                    self.bufferraw.extend(bytes_read)
            # Wait for 0.1 seconds
            time.sleep(0.1)
            # Send the "stop" command to the connected device
            self.vcp.write(b'stop\r\n')
            # Wait for 0.1 seconds
            time.sleep(0.1)
            # Flush and reset the input and output buffers of the connected device
            self.vcp.flush()
            self.vcp.reset_input_buffer()
            self.vcp.reset_output_buffer()
            # Debug log
            logger.debug("<== END reading ")
            logger.debug("prompt : " + str(prompt))
        except ValueError as ve:
            # Log the error
            logger.fatal(ve)
            # Raise the error
            raise

    def _convertpow(self, msb, lsb):
        """ method to convert the 2 bytes of measurement in hex into a float current number """
        data12bits = (msb & 0xF0) >> 4
        datapowerminus16 = ((msb & 0x0F) << 8) + lsb
        result = float(datapowerminus16) * float((16 ** -data12bits))
        return result

    def _process_stream(self):
        """
        this method processes the raw data stored in the 'bufferraw' attribute of the class.
        It converts the raw data into a current measure and retrieves the metadata frames if they exist.
        The converted data and the metadata frames are stored in optimized buffers
        for further processing (cleaning and static measurements).
        """
        logger.debug("<== BEGIN Process")
        self.samples.clear()
        self.metadatas.clear()
        metadata_frame = []
        len_data = len(self.bufferraw)
        i = 0
        while (len_data - i) > 2:
            byte1 = self.bufferraw[i]
            i += 1
            byte2 = self.bufferraw[i]
            if byte1 != 0xf0:
                # convert 2 bytes into current float
                current_float = self._convertpow(byte1, byte2)
                # append a data pair: ( raw data string in hex 'AABB', converted value: current float )
                self.samples.append((f'{byte1:02x}{byte2:02x}', current_float))
                logger.debug(f'{byte1:02x}{byte2:02x}' + " >> " + str(f"{current_float:.5e}"))
                i += 1
            else:
                # catch and get the full metadata frame detected
                logger.fatal("==> metadata detected !! ")
                metadata_frame.append(byte1)
                metadata_frame.append(byte2)
                metadata_id = byte2

                # detect the type and the size of metadata frame
                if metadata_id in STATIC_FRAME_SIZES.keys():
                    frame_size = STATIC_FRAME_SIZES[byte2]
                elif metadata_id in (0xf1, 0xf2):
                    frame_size = 0
                else:
                    frame_size = -1
                logger.fatal("metadata_id : {}".format(format(metadata_id, '#x')))

                # process the metadata frame switch the type
                if frame_size > 0:
                    # a fixed size metadata frame processing ( timestamp or temp metadata for example )
                    while len(metadata_frame) < frame_size:
                        i += 1
                        byte = self.bufferraw[i]
                        metadata_frame.append(byte)

                    # append the metadata frame in deque
                    self.metadatas.append(''.join(f'{x:#x}' for x in metadata_frame))
                    logger.fatal("metadata frame " + ''.join(f'{x:#x}' for x in metadata_frame))
                elif frame_size == 0:
                    # dynamic metadata frame: an error metadata for example
                    while metadata_frame[-4:] != [13, 10, 255, 255]:
                        i += 1
                        byte = self.bufferraw[i]
                        metadata_frame.append(byte)

                    # append the metadata frame in deque
                    self.metadatas.append(''.join(f'{x:#x}' for x in metadata_frame))
                    logger.fatal("metadata frame " + ''.join(f'{x:#x}' for x in metadata_frame))
                else:
                    # unexpected type of metadata : an error or unknown metadata frame type
                    self.metadatas.append(''.join(f'{x:#x}' for x in metadata_frame))
                    logger.fatal("==> unknown metadata frame: error")
                metadata_frame.clear()
                i += 1

        # clean the buffer raw data for the next process
        self.bufferraw.clear()
        logger.debug("<== END Process")

    def clean_samples(self, nbrsamples=1000):
        """
        method to clean the sample buffer of those not needed
        and ensure that the buffer remains a number of 'nbrsamples'.
        """
        while len(self.samples) > nbrsamples:
            self.samples.pop()

    def getsamples(self, nbrsamples=1000):
        """
        method for reading a stream of raw measurement data from the device until a certain number of samples has
        been reached (nbrsamples by default 1000)
        """
        logger.debug("<== BEGIN Acquisation")
        self.temppcb = self.gettemperature('pcb')
        self._read_stream(nbrsamples)

    def process_samples(self, nbrsamples=1000):
        """ This method processes the raw data and clean the converted samples and calculate the needed statics"""
        self._process_stream()
        self.clean_samples(nbrsamples)
        self._updatesamplesmeasurements()
        logger.debug("<== END Acquisation")

    def _updatesamplesmeasurements(self):
        """ this method to get the statics results for the benchmark from the samples """
        self.meansamples = self.get_mean()
        self.maxsamples = self.get_max()
        self.minsamples = self.get_min()
        self.ptpsamples = self.get_ptp()
        self.rmsstdevsamples = self.get_rmsstdev()

    def gettemperature(self, temptype='degc'):
        """ this method to get a temperature from the device switch the type specified as argument """
        if temptype != 'degc' and temptype != 'pcb' and temptype != 'degf':
            raise ValueError(
                f"Invalid type value: {temptype}. The value of the type must be one of the following values [degc, degf, pcb].")

        if temptype == 'degc':
            responses, status = self._ask("get_temp_degc")
            if status:
                return float(responses[3])
            else:
                raise ValueError(f"Error Com with StlinkPwr : {self.com}. please verify the STlink PWR device")
        if temptype == 'degf':
            responses, status = self._ask("get_temp_degf")
            if status:
                return float(responses[3])
            else:
                raise ValueError(f"Error Com with StlinkPwr : {self.com}. please verify the STlink PWR device")
        if temptype == 'pcb':
            responses, status = self._ask("get_temp_pcb")
            if status:
                return float(responses[4])
            else:
                raise ValueError(f"Error Com with StlinkPwr : {self.com}. please verify the STlink PWR device")
        return None

    def get_mean(self):
        """ this method calculate the mean value of samples """
        values = [x[1] for x in self.samples]
        meansmp = statistics.mean(values)
        return meansmp

    def get_max(self):
        """ this method calculate the max value of samples """
        values = [x[1] for x in self.samples]
        maxsmp = max(values)
        return maxsmp

    def get_min(self):
        """ this method calculate the max value of samples """
        values = [x[1] for x in self.samples]
        minsmp = min(values)
        return minsmp

    def get_ptp(self):
        """ this method calculate the peek to peek value of samples """
        values = [x[1] for x in self.samples]
        return max(values) - min(values)

    def get_rmsstdev(self):
        """ this method calculate the RMS SDev value of samples """
        values = [x[1] for x in self.samples]
        mean = statistics.mean(values)
        diffs = [(x - mean) ** 2 for x in values]
        return (sum(diffs) / len(diffs)) ** 0.5

    def close(self):
        """ method to close the serial port of the device """
        try:
            self.vcp.close()
        except:
            raise ValueError(f"Error Com with StlinkPwr : {self.com}. please Replug the STlink PWR device")


if __name__ == "__main__":
    volts_out = [3300]  # in mV
    sampling_tests = [(100000, 1000)]  # (freq [Hz],Nb samples)
    for p in COMPorts.get_com_ports().data:
        if p.description == STLINKPWR_PORTDESC:
            print("StlinkPwr Device @ port: ", p.device)
            try:
                st_device = STLINKPWRCMD(p.device)
                print("\tID: " + st_device.id)
                print("\tFw Version: " + st_device.version)
                st_device.enable_power()
                time.sleep(0.2)
                for vout in volts_out:
                    st_device.setup(volt=vout)
                    print("volt output = ", vout, ' mv')
                    for ts in sampling_tests:
                        print("sampling frequency = ", ts[0], ' Hz')
                        print("Number of samples = ", ts[1])
                        st_device.init_acq(freq=ts[0])
                        startread = time.time_ns()
                        st_device.getsamples(ts[1])
                        st_device.process_samples((ts[1]))
                        print("time process in ms :", round((time.time_ns() - startread) / 100000, 3))
                        print(st_device.samples)
                        print('len of samples :', len(st_device.samples))
                        print("temperature pcb: ", st_device.temppcb)
                        print("mean of samples: ", st_device.meansamples)
                        print("max of samples: ", st_device.maxsamples)
                        print("min of samples: ", st_device.minsamples)
                        print("ptp of samples: ", st_device.ptpsamples)
                        print("rms stdev of samples: ", st_device.rmsstdevsamples)
                        print("metadatas detected :", [m for m in st_device.metadatas])
                        print()
                        print('-*-*' * 10)
                st_device.disable_power()
                st_device.close()
            except ValueError as e:
                print(e)
