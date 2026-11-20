# Validacao da versao leve e do player

## Executado

- Compilacao nativa das bibliotecas FFmpeg, video e guard com VS2010 / XDK instalado.
- Compilacao do executavel NUVIO para Xbox 360.
- Testes de autenticacao, navegacao e addons com transporte HTTP isolado.
- Casos de analogico esquerdo: zona morta, eixos, repeticao inicial de 330 ms e continuacao a cada 110 ms.
- Selecao de fonte HTTP e cabecalhos de provedor; rejeicao de cabecalhos que injetam linhas, Host/Range conflitantes e entradas sem video direto.
- Executavel nativo separado com o mesmo FFPlayer e FFmpeg no Xenia Canary 54386cb. Arquivo: trailer publico Sintel, https://media.w3.org/2010/05/sintel/trailer.mp4.
- Resultado do teste curto: H.264 854x480 a 24 fps, AAC estereo 48 kHz; 52 quadros decodificados, 48 exibidos, zero descartados; sem erro de abertura. Audio com buffer ativo e voz XAudio2 criada.

O teste nao usa login, nao modifica a conta e nao acessa fontes particulares.

## Limites explicitos

O teste anterior no Xenia valida arquivo local e renderizacao/audio. O teste novo de torrent valida o cliente no Windows com um tracker/peer controlado. A tentativa de executar o novo XEX de teste nesta sessao ficou na inicializacao do Xenia, antes de iniciar o titulo; portanto NAO confirma torrent + decoder no Xbox. A reproducao longa em console fisico, HTTP de cada provedor, DHT publico e um TorrServer real continuam dependendo de testes em seu ambiente. Nao foi medido o pico total de RAM de todas as telas do launcher no console; os limites sao aplicados por cache, imagem e fila.

## Memoria

Capas: no maximo 12 texturas de 160x240, 1.76 MiB de pixels brutos, mais alinhamento do driver. Ha um unico fundo e metadados de um titulo em foco. Imagens maiores que 2 milhoes de pixels sao recusadas antes de descomprimir e novos downloads de imagem param quando a memoria fisica disponivel cai abaixo de 32 MiB.

Player: tres quadros YUV, filas com limites de pacotes e de bytes, pacotes individuais limitados a 2 MiB, audio estereo e recusa de video acima de 1280x720. As capas/fundo sao liberados ao entrar no player; sair destroi o player e seus buffers.

## Referencias de codigo

- HG-ERIK/x360-homebrew-libs: 43a3969ec2a5103218a4d7120cceac7ea494e846.
- HG-ERIK/simple360player: 5d235c1fd24c098ccc1b2463c1b4265bf2e7844c.
- NuvioMedia/NuvioTV: protocolo de manifest, stream, proxyHeaders e clientResolve.

A importacao do FFPlayer recebeu limites menores de memoria, limite de resolucao e contagem de bytes por fila. O HTTP local recebeu limite de tamanho, suporte a redirecionamentos de video, verificacao de HTTP Range e supressao de URLs/configuracoes nos logs.

## Cliente torrent integrado - 20/11/2026

Implementacao C++98 propria em `NuvioTorrent.cpp`, baseada nos BEPs 3, 5, 9, 10 e 15 de bittorrent.org. O decoder recebe uma fonte com read/seek; nao e tratado como se pudesse decodificar a URI magnet diretamente.

Verificacoes desta sessao:

- VS2010 host: SHA-1 (incluindo vetor de 1 milhao de caracteres), bencode com limites, rejeicao de entradas invalidas, URI magnet hexadecimal/base32.
- Tracker UDP e peer TCP restritos a 127.0.0.1, usando o trailer aberto Sintel: handshake, negociacao `ut_metadata`, metadados verificados pelo infoHash e 4.372.373 bytes comparados com o arquivo original.
- Busca para frente/tras, limites de arquivo e cancelamento.
- Peer que envia dados corrompidos: cliente recusa a parte antes de devolver bytes ao decoder.
- Torrent com arquivo auxiliar antes do video: `so=1`/`fileIdx=1` seleciona o video e respeita fronteiras de partes entre arquivos.
- Backend de addons: infoHash em clientResolve, trackers e fileIdx preservados; fontes conhecidas como 4K/HEVC rejeitadas com mensagem explicita.

Reproducao: iniciar `python tools/torrent_fixture.py CAMINHO_DO_TRAILER.mp4`; executar `VisualStudio2010/run_torrent_tests.bat "MAGNET_IMPRESSO" "CAMINHO_DO_TRAILER.mp4"`. Opcoes do fixture: `--multi` e `--corrupt`. Para corrupcao, passar tambem `reject-corrupt` ao teste. Encerrar o fixture depois. Ele serve apenas em loopback e e uma ferramenta de teste, nao uma dependencia do aplicativo.

Cache torrent: duas partes verificadas, maximo 4 MiB cada. Metadados ate 1 MiB, 64 peers, 128 KiB por mensagem de peer. Sem cache de filme completo, processo persistente ou alteracao da conta.

Referencias: https://www.bittorrent.org/beps/bep_0003.html, bep_0005.html, bep_0009.html, bep_0010.html e bep_0015.html.

## Alpha 2 validation - November 20, 2026

- Production Http.cpp compiled against a Windows socket/XNet shim: complete MP4 byte comparison, redirects, chunked transfer, byte-range seek, invalid range rejection and truncated-response failure passed. TLS is stubbed out in this host-only transport test; it does not validate HTTPS.
- PC conversion integration passed using synthetic 1080p HEVC and 10-bit PQ/BT.2020 clips served through HTTP. Outputs verified as H.264 1280x720/yuv420p BT.709 SDR and AAC stereo, with stream durations within 120 ms. HTTP authentication and Range checks passed.
- The native player creates conversion jobs on its demux worker, polls pending prepared files and retains the normal seekable MP4 source path. No conversion subprocess runs on Xbox.
- Physical Xbox startup, real-time A/V synchronization on hardware, long playback, public torrent swarms and AV1 input remain unverified. Complete-file conversion is implemented; real-time conversion is not.
