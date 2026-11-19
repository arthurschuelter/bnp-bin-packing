# Problemas do código da LLM


## X. Manutenibilidade

O `main` misturava a coordenação do programa com detalhes de argumentos: comparações de flags, conversões numéricas, validações e mensagens de erro.
Esse bloco longo de condicionais obrigava o leitor a atravessar detalhes da CLI antes de enxergar o fluxo principal: ler, resolver, validar e salvar.

A ideia foi de extrair essa lógica para o `ParseCommandLine`, concentrando as regras em um módulo e entregando as opções validadas ao `main`.
