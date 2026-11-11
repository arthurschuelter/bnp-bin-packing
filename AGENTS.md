# Continuidade deste projeto

Antes de trabalhar, leia `docs/CONTEXTO_CODEX.md`. Ele registra o objetivo,
as decisões, as verificações realizadas e as pendências desta conversa.
Para detalhes históricos, consulte `ai/conversa.md` conforme necessário.
O código e o enunciado têm precedência sobre resumos que possam estar antigos.

- Converse com o usuário em português. O trabalho já está implementado;
  continue a partir do pedido atual, sem reconstruí-lo do zero.
- Preserve `projeto-template/`, que contém o código original fornecido.
- Os fontes ativos estão em `src/`. O pacote acadêmico usa `codigo/` e `ia/`;
  o empacotador faz o mapeamento sem renomear a área de trabalho.
- Preserve os requisitos do PDF: C++17, mestre linear no Gurobi, pricing
  heurístico antes do exato, árvore própria e Ryan–Foster.
- O usuário quer estudar e explicar o algoritmo. Mantenha a documentação
  coerente com mudanças relevantes e diferencie viabilidade de otimalidade.
- Use `.clang-format` e `.clang-tidy`. Após mudanças em C++, execute
  `make format-check` e `make test`; interprete os avisos de `make lint`.
- Confira a instalação local antes de reutilizar caminhos de Gurobi, LLVM
  ou VS Code. Os caminhos salvos foram configurados para um Mac Apple Silicon.
- Há alterações locais, inclusive staged e unstaged. Não as descarte.
