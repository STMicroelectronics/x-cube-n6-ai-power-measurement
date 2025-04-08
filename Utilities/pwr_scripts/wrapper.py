# /*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2024 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/


import stlinkpwr_cmd

stlinkpwr_cmd.COMMAND_INFO['set_trigsrc'] = {
    "command_base": "trigsrc",
    "expected_response": "ack trigsrc",
    "delimiter": "\r\n\r\n"
}

stlinkpwr_cmd.COMMAND_INFO['trigdelay'] = {
    "command_base": "trigdelay",
    "expected_response": "ack trigdelay",
    "delimiter": "\r\n\r\n"
}

class StLinkPwrWrapper(object):
    """docstring for StLinkPwrWrapper"""
    def __init__(self, port):
        super(StLinkPwrWrapper, self).__init__()
        self.cmd = stlinkpwr_cmd.STLINKPWRCMD(port)

    def gettemperature(self):
        return self.cmd.gettemperature()

    def set_volt(self, volt_in_mv):
        responses, status = self.cmd._ask("set_volt", str(volt_in_mv) + "m")
        assert(status == True)

    def set_trigger_mode(self, mode):
        assert(mode == "hw" or mode == "sw")
        responses, status = self.cmd._ask("set_trigsrc", mode)
        assert(status == True)

    def set_trigger_delay(self, delay_in_ms):
        responses, status = self.cmd._ask("trigdelay", str(delay_in_ms) + "m")
        assert(status == True)

    def enable_power(self):
        self.cmd.enable_power()

    def disable_power(self):
        self.cmd.disable_power()

    def capture(self, sample_nb = 20000):
        self.cmd.getsamples(sample_nb)
        self.cmd.process_samples(sample_nb)

        return [d[1] for d in self.cmd.samples]

    def setup(self, volt_in_mv, acq_freq_in_hz):
        self.cmd.setup(volt = volt_in_mv)
        self.cmd.init_acq(freq = acq_freq_in_hz)
        self.set_trigger_mode("hw")
        self.set_trigger_delay(0)
