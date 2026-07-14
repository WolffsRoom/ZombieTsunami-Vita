<p align="center">
  <img src="Assets/ZombieTsunami-Vita.png" alt="Zombie Tsunami - PS Vita Port">
</p>

# Zombie Tsunami — PS Vita Port

Port não oficial de **Zombie Tsunami** para PlayStation Vita, baseado na versão Android do jogo e executado através do **so-loader**.

> Port by **MeninoSung**  
> Patcher by **WolffsRoom**

## Sobre o port

Este projeto foi desenvolvido usando o boilerplate do so-loader. O APK do Zombie Tsunami foi analisado com auxílio de inteligência artificial e, depois de várias compilações, testes e tentativas, foi possível adaptar o jogo para funcionar no PS Vita.

O port não distribui os dados comerciais do jogo. O usuário precisa fornecer seu próprio APK compatível; o patcher extrai os dados necessários e aplica as modificações preparadas para o Vita.

## Requisitos

- PlayStation Vita desbloqueado;
- VitaShell ou outro gerenciador de arquivos compatível;
- [ZombieTsunami-v1.0.vpk](https://github.com/WolffsRoom/ZombieTsunami-Vita/releases/download/v1.0/ZombieTsunami-v1.0.vpk);
- [Patcher v1.0](https://github.com/WolffsRoom/ZombieTsunami-Vita/releases/download/v1.0/Patcher.v1.0.zip);
- APK compatível do **Zombie Tsunami 1.6.0**.

### APK suportado

| Propriedade | Valor |
|---|---|
| Jogo | Zombie Tsunami |
| Versão | 1.6.0 |
| SHA-256 | `B73B109B0FCCDFF8296DA8FE1FE12CCEEEAB17F7DAEE9FE53E229E438299AD42` |

O patcher verifica o tamanho e o SHA-256 do APK antes de iniciar. Outras versões não são aceitas, pois podem conter bibliotecas ou recursos incompatíveis com o port.

## Como gerar os arquivos do jogo

1. Abra a pasta `Release/Patcher v1.0`.
2. Coloque somente um APK compatível dentro da pasta `APK`.
3. Execute `ZombieTsunamiPatcher.exe`.
4. Selecione o idioma da interface.
5. Confira o APK encontrado e confirme o início do processo.
6. Aguarde a verificação e a geração chegarem a 100%.
7. Ao terminar, o patcher criará a pasta:

   ```text
   VitaFiles/zombietsunami
   ```

O patcher oferece interface em inglês, português do Brasil, espanhol, francês, português de Portugal, italiano, russo e japonês.

## Instalação no PS Vita

1. Instale `ZombieTsunami-v1.0.vpk` usando o VitaShell.
2. Copie a pasta `zombietsunami` gerada pelo patcher para `ux0:data/`.
3. Confirme que os arquivos ficaram neste caminho:

   ```text
   ux0:data/zombietsunami/
   ```

4. Inicie **Zombie Tsunami** pela LiveArea.

O resultado esperado inclui as bibliotecas modificadas (`libcgame.so`, `libfmodevent.so` e `libfmodex.so`) e a pasta de assets utilizada pelo jogo.

## Controles

O jogo é controlado inteiramente pela tela de toque do PS Vita, preservando a forma de jogar da versão para dispositivos móveis.

| Controle | Ação |
|:---:|---|
| Tela de toque | Controla toda a interface e as ações do jogo |
| <img src="Assets/SonyButtons/circle.png" height="22" alt="Botão Círculo"> | Pausa o jogo |

## Desenvolvimento do patcher

A pasta `Release/Build_Patch` contém o ambiente usado para criar ou atualizar o patcher:

- APK de referência;
- arquivos finais preparados pelo DEV;
- manifesto e diferenças binárias;
- código-fonte do patcher;
- configuração do PyInstaller;
- `Build_Patcher.bat` para recompilar o executável.

O script de build permite recompilar somente o executável ou regenerar as diferenças binárias antes da compilação.

## Aviso legal

Este é um port não oficial, gratuito e sem fins comerciais. **Zombie Tsunami** e todos os seus recursos pertencem aos respectivos desenvolvedores e detentores dos direitos autorais.

Este repositório não deve ser usado para distribuir APKs ou dados comerciais do jogo. Utilize somente arquivos obtidos legalmente e apoie os desenvolvedores originais.

## Uso de inteligência artificial

Ferramentas de inteligência artificial foram usadas para analisar o APK, auxiliar na investigação de incompatibilidades e apoiar o processo iterativo de compilação e testes. O funcionamento final foi alcançado depois de várias tentativas e ajustes no port.

## Créditos

- **Port by MeninoSung**
- **Patcher by WolffsRoom**
- Boilerplate e base de carregamento: **so-loader**
- Jogo original: seus respectivos desenvolvedores e detentores dos direitos
