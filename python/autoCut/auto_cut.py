import torch 
import typer


from error_pd import infer_error
from logger import logger
from global_settings import *
from utils import find_device


def main():

    device = find_device()

    # add hic figures into list
    hic_figures_list = []
    for root, dirs, files in os.walk(HIC_FIGURE_DIR):
        for file in files:
            if file.endswith(".png"):
                hic_figures_list.append(os.path.join(root, file))
    logger.info(f"Found {len(hic_figures_list)} hic figures.")

    # 
    infer_error_results = infer_error(
        MODEL_CONFIG, 
        PRETRAINED_MODEL,
        HIC_FIGURE_DIR,
        AUTO_CUT_OUTPUT_DIR,
        device=device,
        score = ERROR_SCORE_MIN,
        error_min_len=15000,
        error_max_len=int(2e7),
        iou_score=ERROR_FILTER_IOU_SCORE,
        chr_len=None
    )
    
    return 



if __name__ == "__main__":
    typer.run(main)
