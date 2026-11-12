# Notes
Leia AGENTS.md e docs/CONTEXTO_CODEX.md. Continue deste estado, sem reimplementar do zero, e confira as dependências deste computador.

## 1. Entenda o problema e o que constitui uma resposta válida
Leia as seções 1 a 3 do guia de estudo (docs/GUIA_DE_ESTUDO.md:5) e confira o enunciado em enunciado.pdf (enunciado.pdf). 
Antes de seguir, tente explicar capacidade, classes de conflito e por que cada recipiente válido pode ser representado por um padrão.

## 2. Conheça os dados e a verificação. 
Em binpacking.h (src/binpacking.h:23), estude BinPackingData, depois Conflict, CheckSolution, UpdateBest e TrivialLowerBound. 
A pergunta principal é: como o programa representa uma instância e como decide se uma solução é viável?

### Conflict. 
Percorre o `classes_of` de `i` e `j` até encontrar uma classe igual.
`classes_of` é a "matriz transposta" da matriz de classes, aqui `classes_of[i]` mostra a quais classes `i` pertence.

### CheckSolution.
Os requisitos para uma resposta ser valida são:
1. o item estar em apenas uma bin
2. bin não exceder a capacidade
3. bin não possuir itens com conflito
4. Todo item estar alocado

### UpdateBest.
Atualiza se a quantidade de bins da entrada for menor do que a solução incumbente.
retorna:
- `true`: se atualizou,
- `false`: se não.

### TrivialLowerBound.
Junta todas as bins em uma super bin. Coloque todos os itens dentro, divida pela capacidade.
O resultado seria, por exemplo, 3,7 bins, com item cortado no meio, com item em conflito. Arredonde para cima.
O LowerBound trivial é o máximo entre:
    - essa super bin
    - a quantidade de itens da maior classe de conflitos

Por exemplo:
Classes:
1. A = {0, 1, 2}
2. B = {1, 3, 6}
3. C = {0, 1, 2, 3, 4, 5, 6}
Como a classe C possui 7 itens diferentes, precisaríamos de, pelo menos, 7 bins.

## 3. Siga a entrada e a saída. 
Em main.cpp (src/main.cpp:183), leia main, ReadInstance e WriteSolution. 
Identifique onde os parâmetros entram, onde o solver é chamado e onde a solução é conferida e gravada. 
Não é necessário memorizar todos os detalhes de leitura.


4. Entenda a primeira solução. Em binpacksolver.cpp (src/binpacksolver.cpp:143), comece por SolveBinPacking e siga para RandomizedFFD e FirstFitRound. Veja como a heurística constrói uma solução viável e fornece uma cota superior: a quantidade de recipientes da melhor solução conhecida.
5. Estude geração de colunas antes do código principal. Leia as seções 4 a 7 do guia (docs/GUIA_DE_ESTUDO.md:95): mestre restrito, variáveis duais, custo reduzido, pricing e limite de Farley. Depois volte a BranchAndPrice, em branchprice.cpp (src/branchprice.cpp:240), e localize a criação do mestre, a leitura dos duais e a adição de colunas. O mestre restrito contém apenas parte dos padrões possíveis; seu objetivo, sozinho, não é uma cota inferior válida para o problema inteiro.
6. Aprofunde o pricing. No mesmo arquivo, acompanhe a tentativa gulosa dentro de BranchAndPrice e depois Pricing::exact, dfs e fractional em Pricing (src/branchprice.cpp:140). Procure responder: o que é uma coluna de custo reduzido negativo, por que a heurística é tentada primeiro e o que o pricing exato permite certificar?
7. Feche com a árvore e a prova de otimalidade. Leia as seções 8 e 9 do guia (docs/GUIA_DE_ESTUDO.md:198). No código, estude Node, Groups, Groups::admissible, certifiedBound e volte ao restante de BranchAndPrice. Acompanhe como Ryan–Foster cria os casos “juntos” e “separados”, como essas decisões restringem padrões e como as cotas permitem podar nós.