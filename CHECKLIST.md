# ✅ Checklist — NUVIO.XEX

Esta lista mostra o que já foi programado e o que ainda precisa ser comprovado.

## 1. Código e organização

- [x] Criar projeto nativo em C para Xbox 360 RGH/JTAG
- [x] Renomear projeto e aplicativo para **NUVIO.XEX**
- [x] Criar interface escura com navegação pelo controle
- [x] Configurar Cinemeta, Torrentio e Brazuca Torrents
- [x] Ler manifestos Stremio, incluindo complementos sem catálogo
- [x] Interpretar URLs HTTP, magnet, `infoHash` e `fileIdx`
- [x] Testar parser em computador comum
- [x] Documentar instalação e preparar um GitHub Actions para build

## 2. Compilação e testes reais

- [ ] Confirmar que o **GitHub Actions gera `NUVIO.XEX` sem erros**
- [ ] Baixar o arquivo na seção **Artifacts**
- [ ] Executar o aplicativo no **Aurora do Xbox 360 RGH**
- [ ] Confirmar imagem 720p e resposta dos botões
- [ ] Testar conexão HTTPS no Xbox real
- [ ] Verificar Cinemeta, Torrentio e Brazuca no Xbox real
- [ ] Corrigir travamentos, erros de memória e problemas de rede

## 3. Assistir aos vídeos

- [ ] Criar um player de vídeo e áudio funcional
- [ ] Escolher e implementar BitTorrent nativo **ou** URLs HTTP fornecidas por serviço autorizado, sem PC local
- [ ] Construir buffer eficiente para arquivos grandes
- [ ] Testar H.264 e qualidade 720p no hardware real
- [ ] Implementar pause, retomar e busca no vídeo
- [ ] Selecionar temporadas e episódios (IDs específicos)
- [ ] Legendas e seleção de idioma, incluindo português
- [ ] Testar diferentes filmes e formatos autorizados

## 4. Experiência completa

- [ ] Pôsteres e imagens do catálogo
- [ ] Busca, filtros e gêneros
- [ ] Favoritos e continuar assistindo
- [ ] Configurações e gestão de addons pelo controle
- [ ] Atualizações e instalador simplificado

> Uma função só fica marcada como concluída depois de verificada. **Compilar o `.XEX` não comprova que ele funciona no console ou reproduz vídeo.**
