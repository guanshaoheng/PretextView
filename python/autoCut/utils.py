import torch 
from global_settings import DEBUG
from logger import logger


def find_device(cpu: bool = False):
    if cpu:
        logger.info("Using CPU")
        return torch.device("cpu")
    else:
        logger.info(("Using GPU %s", torch.cuda.get_device_properties()) if torch.cuda.is_available() else "Using CPU")
        return torch.device("cuda" if torch.cuda.is_available() else "cpu")
