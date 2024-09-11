# Пример работы с OO API Firebird на языке C


Если в папкке `include/firebird` отсутвует файл `fb_c_api.h` вы можете создать его самостоятельно.

```bash
cloop FirebirdInterface.idl c-header fb_c_api.h FB_C_API_H I
```

В получившихся файлах не хватает функции `fb_get_master_interface()`. Добавим её определение в файл `fb_c_api.h`.

```cpp
CLOOP_EXTERN_C struct fb_Master* ISC_EXPORT fb_get_master_interface(void);
```

Теперь можно использовать OO API на языке C.

Компиляция примера в Linux

```bash
mkdir build
cmake . -B ./build
cd build
make -j2
```

