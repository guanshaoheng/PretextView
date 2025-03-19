

import os

DEBUG = False
SKIP_STEP_1=True         # Skip the step 1: generate the hic file (juicer) and assemble the genome (3D-DNA)
SKIP_HIC_IMG_GEN=True    # Skip the hic figure generation (hicstraw)


AUTO_CUT_DIR = "/Users/sg35/PretextView/python/autoCut"
MODEL_CONFIG = os.path.join(AUTO_CUT_DIR, "models/cfgs/error_model.py")
PRETRAINED_MODEL = os.path.join(AUTO_CUT_DIR, "models/cfgs/error_model.pth")
# path to hic figures
HIC_FIGURE_DIR = "/Users/sg35/PretextView/build_cmake/PretextViewAI.app/Contents/Resources/auto_curation_tmp/hic_figures"
AUTO_CUT_OUTPUT_DIR = "/Users/sg35/PretextView/build_cmake/PretextViewAI.app/Contents/Resources/auto_curation_tmp/auto_cut_output"
ERROR_FILTER_IOU_SCORE=0.8 # used to filter out the errors with overlap area more than 0.8
ERROR_SCORE_MIN = 0.2

