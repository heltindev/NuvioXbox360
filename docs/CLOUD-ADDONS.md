# Addons na nuvem e interface de TV

## Referência oficial

Protocolo conferido no NuvioMedia/NuvioTV, revisão `ff7fbd747955a1a6108ac23c296b79ad851a14b1`:

- [AddonSyncService.kt](https://github.com/NuvioMedia/NuvioTV/blob/ff7fbd747955a1a6108ac23c296b79ad851a14b1/app/src/main/java/com/nuvio/tv/core/sync/AddonSyncService.kt): `get_sync_owner`, filtragem de perfil, herança e `sync_push_addons`.
- [SupabaseModels.kt](https://github.com/NuvioMedia/NuvioTV/blob/ff7fbd747955a1a6108ac23c296b79ad851a14b1/app/src/main/java/com/nuvio/tv/data/remote/supabase/SupabaseModels.kt): campos de perfil e addons.
- [AddonRepositoryImpl.kt](https://github.com/NuvioMedia/NuvioTV/blob/ff7fbd747955a1a6108ac23c296b79ad851a14b1/app/src/main/java/com/nuvio/tv/data/repository/AddonRepositoryImpl.kt): normalização das URLs, preservando configuração e query.
- [Manifest público](https://catalog.nuvio.tv/manifest.json): categorias, extras, recursos e tipos.

A implementação é C++ para Xbox 360. Não executa Kotlin/Android.

## Fluxo

Após o perfil ser autorizado e selecionado, `/profile/select` define o perfil real. A lista de addons é obtida para seu dono de sincronização. A Home consulta os manifests habilitados, depois os endpoints `/catalog/{type}/{id}.json`. A busca consulta somente catálogos que declaram o extra `search`. Catálogos com outros filtros obrigatórios não são pedidos sem esses filtros.

Os nomes e as divisões das categorias são transportados em TSV interno com cabeçalhos `#catalog` e `#row`. Uma resposta vazia válida limpa a lista; uma falha preserva os dados anteriores. São aceitos IDs como `tmdb:42`, sem presumir prefixo `tt` ou ano obrigatório. O catálogo não importa a base inteira de nenhum serviço.

Metadados, episódios e fontes usam a URL configurada do addon. Credenciais da API Nuvio nunca são enviadas a hosts de addons. URLs de fontes não são exibidas na interface. Ao salvar um título na biblioteca, a origem do addon é preservada.

Antes de ativar, remover ou instalar um addon, o cliente relê o estado remoto. A escrita preserva os outros registros, incluindo URLs legadas e addons desativados, e só confirma sucesso após resposta bem-sucedida. A API oficial substitui a lista inteira; uma alteração simultânea de outro dispositivo entre leitura e escrita ainda pode resultar em último escritor vencedor. Listas maiores que 64 ficam protegidas contra escrita para evitar truncamento. Perfis herdados são somente leitura nesta tela.

O catálogo é carregado em um worker, enquanto a thread principal desenha a tela. Os demais downloads de imagens e comandos ainda usam chamadas síncronas; redes lentas podem pausar esses comandos. Até oito catálogos com 20 títulos cada são consultados por grupo; LB/RB mudam o grupo.

## Validação e pendências

Compilar Release Xbox 360, executar os três testes .bat e testar o manifest/catálogo público. Testes simulados não comprovam acesso real à conta. A autenticação não foi refeita nem houve escrita na conta do usuário neste trabalho.

Próximos testes no aparelho: abrir a mesma pasta com a sessão existente, selecionar perfil, conferir Home e addons do site, buscar um título, salvar/abrir biblioteca, trocar perfil e sair/reabrir para confirmar remoção da sessão. O player nativo foi integrado; a verificacao de PIN segue pendente. Consulte VALIDATION.md para os testes e limites.
