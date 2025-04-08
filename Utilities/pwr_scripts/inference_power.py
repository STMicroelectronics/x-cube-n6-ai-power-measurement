# /*---------------------------------------------------------------------------------------------
#  * Copyright (c) 2024 STMicroelectronics.
#  * All rights reserved.
#  *
#  * This software is licensed under terms that can be found in the LICENSE file in
#  * the root directory of this software component.
#  * If no LICENSE file comes with this software, it is provided AS-IS.
#  *--------------------------------------------------------------------------------------------*/


import csv
import sys
from statistics import mean

import matplotlib.pyplot as plt

VOLT = 3.3
BUCK_EFFICIENCY = 0.87

def filter_inference_samples(rows):
  return list(filter(lambda r: float(r['PD13']) == 1, rows))

def get_time_list(rows):
  return [float(r['time']) for r in rows]

def get_current_list(rows):
  return [float(r['VDD1V8']) for r in rows]

def main(csv_filename):
  with open(csv_filename, newline='') as f:
    reader = csv.DictReader(f)
    rows = list(reader)
    rows = filter_inference_samples(rows)
    inference_time = get_time_list(rows)[-1] - get_time_list(rows)[0]
    mean_current = mean(get_current_list(rows))
    mean_power = VOLT * mean_current * BUCK_EFFICIENCY
    energy = mean_power * inference_time
    print("%3.1f V | %3d%% buck efficiency" %(VOLT, int(BUCK_EFFICIENCY * 100)))
    print("%4.2f ms | %3d mw | %4d uJ" %
      (inference_time * 1000,
       int(mean_power * 1000),
       int(energy * 1000000)))

if __name__ == '__main__':
  main(sys.argv[1])
