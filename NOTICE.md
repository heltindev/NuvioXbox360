# Licencas e origem

O codigo original do projeto NUVIO continua identificado pelo LICENSE Apache-2.0 existente. O executavel combinado que inclui o player deve ser distribuido sob os termos compativeis da GPL-3.0, com seu codigo correspondente, por incluir FFPlayer GPL-3.0 e FFmpeg GPL-2.0-ou-posterior. A licenca completa GPL-3.0 esta em VisualStudio2010/ThirdParty/simple360player/COPYING.

FFPlayer.cpp e FFPlayer.h: HG-ERIK/simple360player, revisao 5d235c1fd24c098ccc1b2463c1b4265bf2e7844c. Modificacoes locais: limites de resolucao, filas por bytes, buffers menores, dois workers de decodificacao e fonte de bytes torrent integrada.

FFmpeg/video/guard: HG-ERIK/x360-homebrew-libs, revisao 43a3969ec2a5103218a4d7120cceac7ea494e846. Licencas e fontes estao em ThirdParty/x360-homebrew-libs. ffglue recebeu verificacao do tamanho dos pacotes e funcao de consulta para limitar filas.

Os demais componentes preservam seus avisos: mbedTLS (Apache-2.0), picojson (BSD), stb e QR encoder (MIT/dominio publico conforme os arquivos). As ferramentas e bibliotecas proprietarias do Xbox XDK nao sao redistribuidas.

NuvioTorrent.cpp/.h: implementacao propria dos protocolos publicos BEP 3/5/9/10/15. A ferramenta tools/torrent_fixture.py usa apenas a biblioteca padrao Python e conteudo aberto fornecido para teste.
