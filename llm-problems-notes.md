# Problemas do código da LLM


## X. Manutenibilidade

O `main` misturava a coordenação do programa com detalhes de argumentos: comparações de flags, conversões numéricas, validações e mensagens de erro.
Esse bloco longo de condicionais obrigava o leitor a atravessar detalhes da CLI antes de enxergar o fluxo principal: ler, resolver, validar e salvar.

A ideia foi de extrair essa lógica para o `ParseCommandLine`, concentrando as regras em um módulo e entregando as opções validadas ao `main`.


## X. Desempate na seleção de nós da árvore

### Problema

A fila de nós abertos (`BestBound`, em `src/branchprice.cpp`) escolhia o nó de menor limitante e, em caso de empate, o de menor `id`, ou seja, o criado primeiro.
No bin packing com conflitos, o limitante é inteiro (`ceil` do valor da relaxação), então empates são muito comuns: muitos nós da fronteira têm o mesmo LB.
Com o desempate por ordem de criação, a busca se comportava como uma busca em largura dentro de cada nível de limitante: espalhava-se pela árvore, demorava a chegar a folhas inteiras e só melhorava o UB mais tarde.

### Alteração

O `Node` ganhou o campo `int depth = 0`.
A raiz é criada com profundidade 0, e os dois filhos de Ryan–Foster ("juntos" e "separados") recebem `node.depth + 1`.
O comparador passou a usar três critérios, em ordem:

```cpp
return a.bound != b.bound   ? a.bound > b.bound   // 1. menor limitante
       : a.depth != b.depth ? a.depth < b.depth   // 2. mais profundo
                            : a.id > b.id;        // 3. criado primeiro
```

Como `std::priority_queue` deixa no topo o "maior" elemento segundo o comparador, `a.depth < b.depth` faz o nó mais profundo sair primeiro.

### Efeito

- A estratégia vira "melhor limitante + mergulho (DFS) nos empates": entre nós igualmente promissores, a busca continua descendo o ramo atual.
- Folhas inteiras tendem a aparecer mais cedo, o que melhora o UB e permite podar mais nós com `bound >= best_value`.
- O critério principal continua sendo o menor limitante, então o LB global certificado não piora.

### Corretude

- A mudança afeta só a **ordem** de exploração, não o conjunto de nós válidos nem os limitantes. Viabilidade e otimalidade se mantêm.
- O comparador continua sendo uma ordem fraca estrita. Assim, a invariante da fronteira continua valendo: o nó é retirado, recolocado na fila e depois removido com `open.pop()` porque volta a ser o topo.
- O `= 0` na declaração de `depth` evita que uma construção futura do `Node` deixe o campo sem valor.

### Verificação

- `make format-check` passa.
- A suíte de `testes/verificar.py` passa (`OK: 187 optima certified`) quando a instância grande `testes/instancias/dificil.txt` (n = 60) fica de fora. Com ela, o oráculo exato, que custa 2^n, estoura a memória.
