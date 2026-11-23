# NUVIO — v0.1.0-alpha.2

Cliente nativo de TV para Xbox 360 RGH/JTAG, inspirado no NuvioTV. Depois de compilar, abra **VisualStudio2010/bin/Release/NUVIO.xex**.

## IMPORTANTE: testar o menu novo no Xbox 360

A branch `main` ainda possui a interface anterior. **Compilar uma pasta
`NuvioXbox360-main` antiga, mesmo com `Build succeeded`, nao incorpora
as alteracoes do PR #2.** Para conferir os quatro icones, as novas configuracoes,
a opacidade dos links incompativeis e o fallback do decoder:

1. Baixe o ZIP de `fix/native-torrent-peer-discovery` em
   https://github.com/heltindev/NuvioXbox360/archive/refs/heads/fix/native-torrent-peer-discovery.zip
   ou use `git clone --branch fix/native-torrent-peer-discovery https://github.com/heltindev/NuvioXbox360.git`.
2. Extraia em **uma nova pasta**. Nao recompila a copia `NuvioXbox360-main`
   que veio do ZIP da `main`.
3. Abra `VisualStudio2010/NUVIO.sln`, selecione `Release | Xbox 360`
   e execute `Rebuild Solution`.
4. Verifique a data do arquivo gerado em
   `VisualStudio2010/bin/Release/NUVIO.xex`. A pasta `VisualStudio2010/Release`
   **nao** e o `OutDir` configurado em `NUVIO.vcxproj`.
5. Guarde uma copia do XEX antigo e substitua o executavel realmente
   iniciado pelo dashboard do Xbox (por exemplo, `default.xex`, se esse
   for o nome utilizado na instalacao). Teste no hardware.

O PR segue experimental: a compilacao nativa desta revisao e o
funcionamento real do decoder ainda precisam de validacao no XDK/Xbox.

## Esta versao

- Interface preta com foco branco, sem animacoes, sem popup por cima dos cartazes.
- Inicio com fundo, metadados e categorias dos addons; grade separada de filmes/series.
- Analogico esquerdo, analogico direito e direcional digital, com zona morta e repeticao.
- Capas de 192x288 com filtragem bilinear em cache de 12 texturas; apenas itens visiveis sao baixados.
- Downloads de capas, metadados, fontes e alteracoes de addons fora da thread de desenho.
- Respostas HTTP de metadados/imagens limitadas a 2 MiB e imagens verificadas antes de descomprimir.
- Player nativo FFmpeg, renderizacao YUV na GPU e audio XAudio2. Filas de pacotes limitadas em quantidade e bytes, tres quadros de video e audio estereo.
- Conta QR, sessao salva, addons por perfil, busca, biblioteca e logout mantidos.
- Barra lateral com exatamente quatro icones: Inicio (casa), Busca (lupa), Biblioteca (pastas) e Configuracoes (engrenagem), com fundo branco arredondado no item selecionado.
- Indicador de carregamento inicial de 0 a 100% e pistas A/B/X/Y com cores nativas do controle.
- Lista de fontes com bandeira de idioma quando o addon informa um idioma reconhecivel; `--` se nao houver informacao confiavel.
- Cliente torrent v1 experimental integrado ao player: metadados e partes verificados com SHA-1, duas partes em cache (ate 8 MiB), sem baixar o filme inteiro em RAM.

## Controles

| Tela | Controles |
| --- | --- |
| Inicio | Direcionais/analogicos: navegar; A: detalhes; X: atualizar; Y: buscar; LB/RB: grupos de categorias |
| Busca | A: tecla; X: apagar; Y: espaco; Start: pesquisar |
| Detalhes | A: escolher episodio/fontes; X: salvar na biblioteca |
| Fontes | A: reproduzir a fonte selecionada; X: atualizar; B: voltar |
| Player | A/Start: pausar/continuar; esquerda/direita: voltar/avancar 10 segundos; B: sair do video. Em erro, A/B volta a lista de fontes. |
| Configuracoes | Cinco categorias: Voce, Visual, Midia, Servicos e Sistema; A abre opcoes implementadas, as demais aparecem apagadas; B: inicio; Y: sair da conta |
| Streaming P2P | A: ligar/desligar P2P, ocultar detalhes ou limpar o cache; B: configuracoes |
| Addons | A: ativar/desativar ou instalar URL; X: remover; Y: sincronizar |

## Conversao automatica

O cliente integra um servidor opcional de conversao para MP4 H.264/SDR ate 720p com AAC estereo. Para iniciar no PC, execute `tools/start_transcoder.cmd`; copie o `nuvio.cfg` gerado junto com o XEX. O PC precisa ficar ligado. O servidor prepara o arquivo completo antes de reproduzir. Leia [docs/TRANSCODING.md](docs/TRANSCODING.md) para configuracao, magnet/TorrServer e limites.

## Configuracoes P2P no Xbox

Em **Configuracoes > Streaming P2P**, o jogador pode habilitar ou bloquear
a abertura de fontes magnet no player, ocultar os detalhes tecnicos durante
o carregamento e limpar o cache de metadados (no maximo 1 MiB).
Os controles sao salvos no proprio Xbox em `game:\\nuvio_p2p.cfg`.
O cache `game:\\nuvio_last_torrent.meta` contem somente metadados SHA-1
verificados do ultimo torrent; os blocos de video continuam limitados a
duas partes temporarias na RAM e sao liberados ao sair da reproducao.

P2P vem habilitado por padrao, mas o usuario pode desativar a qualquer momento.
Essas opcoes nao garantem suporte a novos codecs, uTP ou torrents v2-only.

## Formatos e torrents

Escolha fontes **H.264/AVC ate 720p**, de preferencia com audio AAC estereo. A biblioteca do player tambem oferece codecs legados, mas o teste automatizado desta alteracao validou H.264/AAC em MP4. HEVC/H.265, AV1, DRM e 1080p nao sao garantidos; fontes acima de 1280x720 sao recusadas para manter o consumo limitado.

Links HTTP/HTTPS dos addons entram diretamente no player, com cabecalhos do provedor, redirecionamentos limitados e HTTP Range. O player agora passa um nome de arquivo sem credenciais ou parametros da URL ao detector de conteiner do FFmpeg, para ajudar a reconhecer formatos com extensao em links HTTP. Isso nao adiciona decoders novos ao binario.

Fontes magnet/infoHash e `torrent://hash/fileIdx` agora entram no cliente BitTorrent **dentro do XEX**, que busca peers por tracker UDP/HTTP ou DHT IPv4, obtem metadados por `ut_metadata`, verifica as partes e entrega os bytes ao FFmpeg. O indice `fileIdx` do addon seleciona o arquivo correto de um torrent com varios arquivos. Nao precisa de um processo rodando no PC para esse caminho.

Nesta revisao, o cliente consulta mais de um tracker, usa trackers publicos apenas quando o magnet nao fornece nenhum e tenta peers adicionais via DHT quando os primeiros nao oferecem metadados. O indice `fileIdx` continua selecionando o arquivo dentro do torrent.

O progresso de **0 a 100%** mostrado durante a abertura representa fases de preparacao e o buffer inicial; **nao** representa a porcentagem do filme baixado. O player P2P busca partes verificadas sob demanda, sem armazenar o filme inteiro. Ja a conversao opcional no PC prepara o arquivo convertido completo antes de reproduzir.

O erro `Unknown container format` agora gera orientacao em portugues; se houver falha na leitura do torrent, o player prioriza a mensagem do torrent. MP4, MKV e AVI so tocam quando o demuxer e o codec existem no FFmpeg embarcado, dentro do limite de 720p. Nao ha como garantir HEVC, AV1, DRM ou todos os formatos com o FFmpeg 0.7 atual.

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
