# NUVIO — projeto Xbox 360

Abra `NUVIO.sln` e compile **Release | Xbox 360** no Visual Studio 2010 com XDK instalado. O executável é `bin/Release/NUVIO.xex` (conforme `OutDir` em `NUVIO.vcxproj`).

Consulte [README principal](../README.md) para operação, controles e limites, e [integração dos addons](../docs/CLOUD-ADDONS.md) para arquitetura e referências oficiais. Os testes são `run_core_tests.bat`, `run_auth_tests.bat`, `run_cloud_tests.bat`, `run_http_tests.bat` e `run_torrent_tests.bat`.

As dependências em ThirdParty são necessárias para HTTPS, TLS, JSON, imagens e QR. Preserve-as e suas licenças. O nome de distribuição do aplicativo é NUVIO.

Os avisos `LNK4099` de PDB ausente nas bibliotecas de terceiros não impedem o build; confirme `0 failed` na saída do Visual Studio e teste o XEX gerado no console.
