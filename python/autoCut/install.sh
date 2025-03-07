#!/bin/bash


# wget https://github.com/open-mmlab/mmdetection/archive/refs/tags/v2.12.0.tar.gz

# tar -zxvf v2.12.0.tar.gz
# cd mmdetection-2.12.0
# cd python/autoCut/models
conda create -n auto_cut python=3.9.16 cython openmim typer  -y
conda activate auto_cut
pip install torch==1.10.1 torchvision==0.11.2 matplotlib==3.5.3 pycocotools==2.0.6 numpy==1.24.3 six==1.16.0 terminaltables==3.1.10 timm==0.6.13 tomli==2.0.1 platformdirs==4.3.6 pandas==2.0.1 openpyxl==3.0.10
mim install mmengine mmcv-full==1.3.7 
pip install -e ./swin
