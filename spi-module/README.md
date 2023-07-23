<!--
 * @Date: 2023-07-23 21:21:58
 * @LastEditors: 974782852@qq.com
 * @LastEditTime: 2023-07-23 21:21:59
 * @FilePath: \riscv_milkv-duo\spi-module\README.md
-->
## 说明
- 使用spi platform增加user driver

## 设备树
`build\boards\cv180x\cv1800b_milkv_duo_sd\dts_riscv\cv1800b_milkv_duo_sd.dts`
```
&spi2 {
    status = "okay";
	/delete-node/ spidev@0;
    spi_channel: spi_channel@0{
        compatible = "litchicheng,spi_channel";
		reg = <0>;
        status = "okay";
        spi-max-frequency = <8000000>;
        spi-cpol;
        spi-cpha;
        buswidth = <8>;
	};
};
```

## config
```
CONFIG_SPI=y
CONFIG_SPI_MASTER=y
CONFIG_SPI_DESIGNWARE=y
CONFIG_SPI_DW_MMIO=y
CONFIG_SPI_SPIDEV=y
```

## 测试
### 模块和dev
![模块和dev](./pic/module&dev.png)
### app测试输出
![app测试输出](./pic/app-test-output.png)
