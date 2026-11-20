# NUVIO — v0.1.0-alpha.2

Cliente nativo de TV para Xbox 360 RGH/JTAG, inspirado no NuvioTV. Abra **VisualStudio2010/Release/NUVIO.xex**.

## Esta versao

- Interface preta com foco branco, sem animacoes, sem popup por cima dos cartazes.
- Inicio com fundo, metadados e categorias dos addons; grade separada de filmes/series.
- Analogico esquerdo, analogico direito e direcional digital, com zona morta e repeticao.
- Capas de 160x240 em um cache de somente 12 texturas; apenas itens visiveis sao baixados.
- Downloads de capas, metadados, fontes e alteracoes de addons fora da thread de desenho.
- Respostas HTTP de metadados/imagens limitadas a 2 MiB e imagens verificadas antes de descomprimir.
- Player nativo FFmpeg, renderizacao YUV na GPU e audio XAudio2. Filas de pacotes limitadas em quantidade e bytes, tres quadros de video e audio estereo.
- Conta QR, sessao salva, addons por perfil, busca, biblioteca e logout mantidos.
- Barra lateral com botoes de 60 pixels, icones de 40 pixels e nomes ao focar.
- Cliente torrent v1 experimental integrado ao player: metadados e partes verificados com SHA-1, duas partes em cache (ate 8 MiB), sem baixar o filme inteiro em RAM.

## Controles

| Tela | Controles |
| --- | --- |
| Inicio | Direcionais/analogicos: navegar; A: detalhes; X: atualizar; Y: buscar; LB/RB: grupos de categorias |
| Filmes e series | Y: trocar Filmes/Series/Todos; Start: buscar; A: detalhes |
| Busca | A: tecla; X: apagar; Y: espaco; Start: pesquisar |
| Detalhes | A: escolher episodio/fontes; X: salvar na biblioteca |
| Fontes | A: reproduzir a fonte selecionada; X: atualizar; B: voltar |
| Player | A/Start: pausar/continuar; esquerda/direita: voltar/avancar 10 segundos; B: sair do video |
| Configuracoes | Addons, trocar perfil, sincronizar conta e sair; Y: sair da conta |
| Addons | A: ativar/desativar ou instalar URL; X: remover; Y: sincronizar |

## Conversao automatica

O cliente integra um servidor opcional de conversao para MP4 H.264/SDR ate 720p com AAC estereo. Para iniciar no PC, execute `tools/start_transcoder.cmd`; copie o `nuvio.cfg` gerado junto com o XEX. O PC precisa ficar ligado. O servidor prepara o arquivo completo antes de reproduzir. Leia [docs/TRANSCODING.md](docs/TRANSCODING.md) para configuracao, magnet/TorrServer e limites.

## Formatos e torrents

Escolha fontes **H.264/AVC ate 720p**, de preferencia com audio AAC estereo. A biblioteca do player tambem oferece codecs legados, mas o teste automatizado desta alteracao validou H.264/AAC em MP4. HEVC/H.265, AV1, DRM e 1080p nao sao garantidos; fontes acima de 1280x720 sao recusadas para manter o consumo limitado.

Links HTTP/HTTPS dos addons entram diretamente no player, com cabecalhos do provedor, redirecionamentos limitados e HTTP Range.

Fontes magnet/infoHash agora entram no cliente BitTorrent **dentro do XEX**, que busca peers por tracker UDP/HTTP ou DHT IPv4, obtem metadados por `ut_metadata`, verifica as partes e entrega os bytes ao FFmpeg. O indice `fileIdx` do addon seleciona o arquivo correto de um torrent com varios arquivos. Nao precisa de um processo rodando no PC para esse caminho.

Esta implementacao e experimental: aceita torrents v1/SHA-1 por TCP sem criptografia de protocolo, metadados de ate 1 MiB e partes de ate 4 MiB. Nao implementa uTP, BitTorrent v2, IPv6 ou conversao de codec. A velocidade depende de peers compativeis e das partes disponiveis. O teste controlado no Windows passou; o novo caminho completo no Xbox/emulador ainda precisa ser confirmado.

Se voce ja utiliza TorrServer, a configuracao opcional abaixo continua encaminhando o magnet para ele:

```
torrent_server=https://SEU-SERVIDOR
```

Nao existe servidor publico predefinido. Uma fonte 4K/H.265 continua exigindo conversao externa para formato leve; resolver o torrent nao muda o codec do video.

## Verificacao

O player foi executado no Xenia Canary com um arquivo de teste H.264 854x480/AAC 48 kHz estereo: 52 quadros decodificados, 48 exibidos, zero descartados no teste curto. O novo cliente torrent passou no Windows com tracker UDP e peer TCP controlados: arquivo completo de 4.372.373 bytes, verificacao de partes, busca e recusa de dados corrompidos. Esse teste e separado do teste anterior de arquivo local no Xenia. Navegacao, entrada analogica, autenticacao, addons e selecao de fontes possuem testes locais. Isso nao comprova todos os codecs, provedores nem desempenho do Xbox fisico. Veja [docs/VALIDATION.md](docs/VALIDATION.md).

## Compilacao

Abra `VisualStudio2010/NUVIO.sln` no VS2010 com XDK e selecione Release | Xbox 360. `build_decoder.ps1` compila as dependencias do decoder e `build_native_tls.ps1` compila HTTPS/TLS quando necessario. Preserve ThirdParty e suas licencas.

O cache `nuvio_session.dat` e privado da conta. Nao o compartilhe. O aplicativo permanece nao oficial. Consulte [NOTICE.md](NOTICE.md) para as licencas do player combinado.

Source build: run `VisualStudio2010/bootstrap_dependencies.ps1`, then `build_native_tls.ps1`, `build_decoder.ps1` and build `NUVIO.vcxproj`. See [release notes](docs/RELEASE_NOTES.md) and [checklist](docs/CHECKLIST.md).
