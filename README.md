<div align="center">
  <img src="assets/banner.svg" alt="NUVIO.XEX — aplicativo experimental para Xbox 360" width="100%" />

  # NUVIO.XEX 🎮

  **Um aplicativo de mídia experimental para Xbox 360 RGH/JTAG, feito para a comunidade.**

  [![Compilar XEX](https://github.com/heltindev/NuvioXbox360/actions/workflows/build-xex.yml/badge.svg)](https://github.com/heltindev/NuvioXbox360/actions/workflows/build-xex.yml)
  ![Plataforma](https://img.shields.io/badge/Xbox%20360-RGH-12b886)
  ![Versão](https://img.shields.io/badge/versão-0.3%20experimental-9c7cff)
</div>

> [!IMPORTANT]
> **Ainda não é um app para assistir filmes.** Esta versão tem interface, rede e leitura de catálogos/fontes, mas **não possui player de vídeo nem cliente BitTorrent**. O `.XEX` gerado serve para testar a inicialização e a navegação no Xbox 360.

## 🌟 O que já existe?

| Recurso | Situação |
|---|---|
| Interface escura em 720p e navegação pelo controle | Código pronto; falta testar no Xbox |
| Catálogo Cinemeta | Implementado; falta testar no Xbox |
| Seleção de Torrentio ou Brazuca Torrents | Configurada no código |
| Leitura de fontes HTTP, magnet, `infoHash` e `fileIdx` | Parser testado no computador |
| Reproduzir vídeo/áudio | ❌ Ainda não implementado |
| Baixar torrent diretamente no Xbox | ❌ Ainda não implementado |

É um projeto **não oficial e independente**, inspirado na organização do [Nuvio original](https://github.com/NuvioMedia/NuvioMobile) e no [protocolo Stremio](https://github.com/Stremio/stremio-addon-sdk/blob/master/docs/protocol.md). O Nuvio Mobile usa Kotlin Multiplatform e não pode ser compilado diretamente para Xbox 360: este projeto usa C com o SDK aberto [xsdk](https://github.com/xenosworks/xsdk).

## ⬇️ Como baixar o teste `NUVIO.XEX` sem programar

1. Abra [**Actions → Build NUVIO.XEX**](https://github.com/heltindev/NuvioXbox360/actions/workflows/build-xex.yml).
2. Clique em **Run workflow** e confirme, caso não exista uma execução em andamento.
3. Aguarde a conclusão. A primeira compilação pode demorar porque também constrói o compilador do Xbox 360.
4. Se a execução ficar **verde ✅**, clique nela e procure **Artifacts → NUVIO-XEX-test**.
5. Baixe o ZIP, que deverá conter `NUVIO.XEX`, `cacert.pem`, `addon_url.txt` e `stream_addons.txt`.

Se ficar **vermelho ❌**, abra a etapa que falhou para ver o erro. A compilação automatizada ainda precisa ser validada; não significa que exista um `.XEX` pronto para instalar.

### ⚡ Por que a primeira compilação demora mais?

O Xbox 360 exige um compilador PowerPC especial. Como não encontramos uma distribuição pré-compilada oficial do xsdk, o GitHub precisa construir esse compilador na primeira execução. **Depois que o compilador e as ferramentas gráficas forem criados, o Actions salva cada um no cache**, para acelerar as próximas tentativas, mesmo que o aplicativo ainda precise de correções.

Na tela do Actions, as etapas aparecem com nomes em português e emojis. Se algo falhar, consulte o resumo **“O NUVIO.XEX ainda não ficou pronto”** no final da execução.

## 🕹️ Como testar no Xbox 360 RGH com Aurora

1. Prepare um pendrive FAT32 ou utilize o gerenciador de arquivos do Aurora.
2. Crie uma pasta, por exemplo `Hdd1:\Apps\NUVIO`.
3. Copie **todos os quatro arquivos do ZIP para a mesma pasta**.
4. Ajuste data e hora do console (necessárias para HTTPS) e conecte-o à internet.
5. Execute `NUVIO.XEX` pelo gerenciador do Aurora.

| Botão | Função |
|---|---|
| ↑ / ↓ | Navegar |
| A | Selecionar |
| B | Voltar |
| Y | Atualizar catálogo |
| Back | Sair |

O fluxo previsto é **títulos → escolher Torrentio/Brazuca → fontes → tela informativa**. Reconhecer uma fonte **não** significa reproduzi-la nesta versão.

## 📋 O que falta fazer?

Confira a **[CHECKLIST.md](CHECKLIST.md)** para acompanhar tudo até o aplicativo final.

## 📁 Entenda os arquivos

| Arquivo | Para que serve? |
|---|---|
| `src/main.c` | Interface, tela e controle |
| `src/stremio.c` | Entende catálogos e fontes |
| `src/net.c` | Conexão HTTP/HTTPS |
| `addon_url.txt` | Catálogo Cinemeta |
| `stream_addons.txt` | Complementos Torrentio e Brazuca |
| `.github/workflows/build-xex.yml` | Compilação automática |
| `tests/` | Testes no computador |
| `third_party/cjson/` | Biblioteca para ler JSON |

Para programadores: [docs/DESENVOLVIMENTO.md](docs/DESENVOLVIMENTO.md).

## 🤝 Créditos e licenças

- Projeto não afiliado à NuvioMedia, Microsoft, Xbox ou Stremio.
- Nuvio Mobile é distribuído sob **GPL-3.0**. Seu código foi consultado como referência, **não foi copiado** para este repositório.
- O código próprio deste repositório segue a licença já existente [Apache-2.0](LICENSE). `cJSON` mantém o aviso MIT em seus arquivos.
- `xsdk`, `curl-xenon` e `mbedtls-xenon` são baixados durante a compilação e mantêm suas licenças.
- Use apenas fontes de conteúdo que você tenha direito de acessar.
