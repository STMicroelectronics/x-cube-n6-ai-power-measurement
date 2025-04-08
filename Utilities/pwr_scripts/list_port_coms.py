# /*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2024 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/


STLINKPWR_PORTDESC = "STMicroelectronics STLink Virtual COM Port PWR"

class COMPorts(object):
    """docstring for COMPorts"""
    def __init__(self, arg):
        super(COMPorts, self).__init__()

    @staticmethod
    def get_description_by_device(device: str):
        return STLINKPWR_PORTDESC
