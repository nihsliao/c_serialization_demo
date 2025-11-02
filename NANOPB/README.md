# nanopb library introduction

## First of all
- **!!請不要手動去修改 STRUCTURE.pb.\* 檔案內容!!**
    - **請修改 `.proto` OR `.options`** 再以指令重新產生
- `nanopb` 為 zlib license
- 詳細可以參考 `nanopb` 下的說明
    - 使用 [Nanopb 0.4.9.1](https://github.com/nanopb/nanopb/releases/tag/nanopb-0.4.9.1)

### Requirement
- Install `python`, `protobuf`

##### 可選： python enviroment
```shell
python3 -m venv path/to/venv
source path/to/venv/bin/activate
python3 -m pip install protobuf
```
### Steps
1. Create `.proto` and `.options`(optional) 並移至 `nanopb` 檔案夾中
    - 可以參考 [sample_structure.h](./nanopb/sample_structure.proto)
2. 在 `nanopb` 檔案夾中執行 command 產生 `myprotocol.pb.c` & `myprotocol.pb.h`
    ```shell
    python generator/nanopb_generator.py myprotocol.proto  # For source checkout
    generator-bin/nanopb_generator myprotocol.proto        # For binary package
    ```
3. 在使用的地方 `include`
    ```c
    #include "myprotocol.pb.h"
    ```
4. `Makefile` 添加 library & 產生的 `.c` file
    ```shell
    NANOPB = NANOPB/nanopb/*.c
    SRC = main.c $(NANOPB)
    ```