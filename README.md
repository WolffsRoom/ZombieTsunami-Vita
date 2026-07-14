# Zombie Tsunami Vita — bring-up port

Port experimental de **Zombie Tsunami 1.7.0 (versionCode 31)** para PlayStation Vita, baseado no [soloader-boilerplate](https://github.com/v-atamanenko/soloader-boilerplate).

Este repositório contém somente o loader e código de compatibilidade. O APK e os dados proprietários do jogo não são redistribuídos.

## Estado

O bring-up inicial está implementado:

- carregamento ARMv7/EABI5 de `libfmodex.so`, `libfmodevent.so` e `libcgame.so`, nessa ordem;
- resolução estática de todos os 357 imports de `libcgame.so` e dos imports das duas bibliotecas FMOD;
- inicialização do renderer nativo em 960×544;
- loop de `nativeRender`, touch frontal e botões de voltar/pausa;
- stubs FalsoJNI para serviços Android sem equivalente no Vita: anúncios, Facebook, Parse, billing e notificações;
- preparação reproduzível dos dados a partir do APK original.

Ainda é um **protótipo de bring-up**, não uma release jogável confirmada. Ele precisa ser compilado com VitaSDK-softfp e executado em um Vita real para coletar os primeiros logs FalsoJNI/crash dump. Os callbacks Java de decodificação de bitmap permanecem como stubs e podem precisar de implementação caso o jogo os use durante a inicialização.

## APK suportado

Arquivo analisado: `Zombie Tsunami 1.7.0 (31).apk`

```text
SHA-256: 125A911E4BE945F18184A726445627AA52F01595718CF696E84293D182CEF919
ABI: armeabi-v7a
Pacote: net.mobigame.zombietsunami
```

## Preparar os dados

No PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\prepare-data.ps1 "C:\caminho\Zombie Tsunami 1.7.0 (31).apk"
```

O script valida o SHA-256 e cria `prepared-data` com esta estrutura:

```text
prepared-data/
├── assets/
├── libcgame.so
├── libfmodevent.so
└── libfmodex.so
```

Copie o conteúdo da pasta para:

```text
ux0:data/zombietsunami/
```

## Compilar

Requisitos:

- [VitaSDK-softfp](https://github.com/vitasdk-softfp) com `VITASDK` configurado;
- dependências usadas pelo boilerplate, incluindo vitaGL, vitashark, OpenSLES e bibliotecas de áudio;
- `kubridge.skprx` instalado no Vita.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

O build gera `build/zombie_tsunami.vpk`. Para o primeiro teste, use `Debug`: ele habilita os logs do loader. Para logging JNI completo, descomente `FALSOJNI_DEBUGLEVEL=0` em `CMakeLists.txt`.

## Controles iniciais

- touch frontal: toque do Android;
- Círculo: voltar;
- Start: alternar pausa/retomada;
- X, Quadrado e Triângulo: callbacks dos três botões de diálogo do jogo.

Os analógicos ainda não estão mapeados porque o jogo original é controlado por touch.

## Primeiro teste no Vita

1. Instale `kubridge.skprx` e reinicie o Vita.
2. Instale `zombie_tsunami.vpk`.
3. Copie os dados para `ux0:data/zombietsunami/`.
4. Inicie o jogo e capture o output de debug (TTY/PSMLogUSB) ou o crash dump.
5. Procure principalmente por `GetMethodID`, `GetFieldID`, `Unknown symbol` ou pelo primeiro PC de crash.

Veja [PORTING_NOTES.md](PORTING_NOTES.md) para os detalhes técnicos e os próximos pontos de investigação.

## Créditos e licença

O loader deriva do projeto MIT `soloader-boilerplate`, de Volodymyr Atamanenko e contribuidores. FalsoJNI está vendorizado em `lib/falso_jni` sob sua licença MIT. Os direitos de Zombie Tsunami e de seus assets pertencem aos respectivos titulares.
