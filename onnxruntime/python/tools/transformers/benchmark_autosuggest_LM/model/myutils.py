import torch
import numpy as np

def outFileHandler(fh):
    global common_fh
    global counterset
    global total_infer_time

    total_infer_time = 0
    counterset = False
    common_fh = fh
    common_fh.write("TotalInferenceTime\tCounter\tTotalModelTime\tTotalSearchTime\tResult\tTotalQueryTime\n")

