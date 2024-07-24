# Пример работы с объкноориентированном API Firebird в C

Создание интерфейсов для языка C.

```bash
cloop FirebirdInterface.idl c-header fb_api.h FB_OOAPI_H fb_
cloop FirebirdInterface.idl c-impl fb_api_impl.c fb_api.h fb_
```

Компиляция в Linux

```
mkdir build
cmake . -B ./build
cd build
make -j2
```

