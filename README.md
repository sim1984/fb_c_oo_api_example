# Пример работы с OO API Firebird на языке C

Создание интерфейсов для языка C.

```bash
cloop FirebirdInterface.idl c-header fb_api.h FB_OOAPI_H fb_
cloop FirebirdInterface.idl c-impl fb_api_impl.c fb_api.h fb_
```

В получившихся файлах не хватает функции `fb_get_master_interface()`. Добавим её определение в файл `fb_api.h`.

```c
CLOOP_EXTERN_C struct fb_Master* ISC_EXPORT fb_get_master_interface();
```

Теперь можно использовать OO API на языке C.

Компиляция примера в Linux

```bash
mkdir build
cmake . -B ./build
cd build
make -j2
```

