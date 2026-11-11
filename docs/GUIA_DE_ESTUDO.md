# Guia de estudo e verificação do Projeto 2

Este guia explica a implementação que está neste repositório. A referência dos requisitos é o `enunciado.pdf`, principalmente as seções 10.3–10.7. Leia com o código aberto: o objetivo é conseguir justificar as decisões, reproduzir os resultados e modificar o programa.

## 1. Como saber se a resposta está correta?

Há três perguntas diferentes:

| Pergunta | Como verificar | O que isso permite concluir |
|---|---|---|
| Todos os itens foram empacotados respeitando as regras? | Verificador de partição, capacidade e conflitos | A solução é **viável** |
| Existe uma solução melhor? | Limitante inferior válido igual ao valor da solução | A solução é **ótima**, dentro das tolerâncias numéricas |
| O programa implementou corretamente essa prova? | Revisão dos invariantes, comparação com outro algoritmo e testes de interrupção | Evidência sobre a correção da implementação |

Encontrar um empacotamento de quatro recipientes prova apenas `OPT <= 4`. Para concluir `OPT = 4`, também é necessário provar `OPT >= 4`.

O arquivo de saída contém o limitante produzido pelo próprio programa. Ver `LB=UB` nesse arquivo é a conclusão do algoritmo; a igualdade, isoladamente, não verifica se o código calculou LB corretamente. Para instâncias pequenas, podemos calcular o ótimo por outro algoritmo e comparar.

### Experimento inicial

Na raiz da área de trabalho:

```bash
make run
python3 testes/conferir_solucao.py -f testes/instancias/exemplo.txt -o saida.txt --exato
make test
```

O primeiro comando compila se necessário e resolve o exemplo. O segundo confere a saída usando Python e calcula o ótimo por uma programação dinâmica independente do branch-and-price. O terceiro executa a suíte completa.

Resultado esperado para o exemplo:

```text
VIÁVEL: 4 recipientes; partição, capacidade e conflitos conferidos.
Ótimo independente: 4; limitante informado: 4.
SOLUÇÃO ÓTIMA confirmada pelo oráculo.
```

A composição e a ordem dos recipientes podem variar: o que precisa coincidir é a validade e o número ótimo.

Os comandos acima usam a estrutura local `src/`, `testes/`, `ai/`, `docs/`. O ZIP de entrega mapeia `src/` para `codigo/` e `ai/` para `ia/`, conforme o PDF. No ZIP extraído, use `make -C codigo run` e `make -C codigo test` a partir da raiz do pacote. Os verificadores Python reconhecem as duas estruturas.

## 2. O problema, antes do algoritmo

A entrada contém `n` itens, capacidade `C`, tamanhos positivos `s_i <= C` e classes de conflito `S_t`. Dois itens em uma mesma classe precisam ficar separados. Classes podem se sobrepor; um item pode pertencer a várias ou a nenhuma.

Um recipiente válido `B` é um conjunto não vazio de itens tal que:

\[
\sum_{i\in B}s_i\le C,
\qquad |B\cap S_t|\le1\quad\text{para cada classe }t.
\]

Uma solução é uma **partição** dos itens em recipientes válidos: cada item aparece exatamente uma vez. Minimiza-se a quantidade de recipientes.

Uma classe `{1,2,3}` proíbe os três pares `{1,2}`, `{1,3}`, `{2,3}`. Classes `{1,2}` e `{2,3}`, por outro lado, não proíbem automaticamente `{1,3}`. Compartilhar um caminho de conflitos não é o mesmo que compartilhar uma classe.

Definições usadas no código:

- **Incumbente:** melhor solução inteira viável encontrada.
- **UB:** quantidade de recipientes dessa solução; portanto `OPT <= UB`.
- **LB:** limite inferior provado; portanto `LB <= OPT`.
- **Coluna/padrão:** conjunto de itens que pode formar um recipiente.
- **Nó:** subproblema com decisões adicionais de itens juntos ou separados.

As cotas iniciais são:

\[
LB_0=\max\left\{\left\lceil\frac{\sum_i s_i}{C}\right\rceil,
\max_t |S_t|\right\}.
\]

Sem classes, o segundo termo é zero. As cotas são válidas porque cada recipiente tem capacidade `C` e cada classe precisa distribuir seus itens em recipientes distintos.

## 3. Por que uma variável por recipiente válido?

Seja `Bcal` a família de todos os recipientes válidos não vazios. A formulação do enunciado é:

\[
\min\sum_{B\in\mathcal B}x_B
\]
\[
\sum_{B\ni i}x_B=1\quad\forall i,
\qquad x_B\in\{0,1\}.
\]

`x_B=1` significa usar o recipiente `B`. As igualdades exigem cobrir cada item exatamente uma vez. O custo de qualquer coluna é 1, pois cada recipiente conta uma unidade.

O número de recipientes válidos pode ser exponencial em `n`. Armazenar todos antes de resolver seria caro. O programa começa com alguns e gera outros quando necessários.

Para obter uma relaxação linear, substituímos as condições binárias por `x_B >= 0`. Não é necessário impor `x_B <= 1`: como cada coluna contém pelo menos um item, sua igualdade de cobertura já implica esse limite.

**Papel do Gurobi:** resolver os PLs com as colunas disponíveis. A árvore, o pricing e a ramificação são implementados pelo programa. Em `branchprice.cpp`, a criação de variáveis usa `GRB_CONTINUOUS`. O pricing exato desta implementação também não usa MIP.

## 4. Como começa a busca

A função `SolveBinPacking`, em `src/binpacksolver.cpp`, instala primeiro a solução de `n` recipientes unitários. Ela é válida para qualquer entrada aceita, inclusive quando `T=0`.

Depois executa a heurística probabilística do código-base. Cada rodada:

1. Sorteia uma tendência `beta` em `[0,1]`.
2. Calcula a prioridade de cada item por tamanho, participação em classes e aleatoriedade, conforme a fórmula do PDF.
3. Ordena os itens por prioridade decrescente.
4. Coloca cada item no primeiro recipiente em que cabe sem conflito; se não houver, abre outro.

Isso produz soluções viáveis, mas não prova otimalidade. Uma rodada incompleta por falta de tempo é descartada. A heurística usa até 1000 rodadas e até `min(0,25 s; 5% de T)` para reservar orçamento à busca exata.

O banco inicial de colunas recebe todos os recipientes unitários e os recipientes da melhor solução heurística. Se a heurística atingir `LB_0`, o ótimo já foi provado e não é necessário abrir a árvore.

## 5. Mestre restrito, dual e custo reduzido

O **problema mestre restrito (PMR)** contém só as colunas já conhecidas. Ele usa as mesmas igualdades da formulação completa, mas com uma família menor de padrões.

Por ser um problema de minimização com menos opções:

\[
z_{PMR}\ge z_{LP}.
\]

Aqui `z_LP` é o ótimo da relaxação com todas as colunas. Atenção: não existe uma relação que permita concluir `z_PMR <= OPT` em geral. Se o mestre contém apenas unitários, pode custar `n`, mesmo que exista um empacotamento com muito menos recipientes. Usar o objetivo do PMR diretamente como LB antes de certificar o pricing pode podar soluções ótimas.

Para as igualdades de cobertura, o dual completo é:

\[
\max\sum_i u_i,
\qquad \sum_{i\in B}u_i\le1\quad\forall B\in\mathcal B,
\qquad u_i\in\mathbb R.
\]

Os duais são livres, pois as restrições primais são igualdades. O Gurobi retorna esses valores no atributo `Pi`.

O custo reduzido de uma coluna é:

\[
\bar c_B=1-\sum_{i\in B}u_i.
\]

Exemplo aritmético: uma coluna válida com duais `0,7` e `0,6` soma `1,3` e tem custo reduzido `-0,3`. Ela viola uma restrição dual ausente e deve ser oferecida ao mestre. Sua inclusão pode melhorar o objetivo; degenerescência pode impedir uma melhora imediata estrita.

## 6. O pricing procura o quê?

O pricing resolve:

\[
\alpha^*=\max_{B\text{ válido}}\sum_{i\in B}u_i.
\]

Isso é uma mochila com conflitos: tamanhos são os pesos, capacidade é `C` e duais são os valores. Há coluna melhorante quando `alpha* > 1`.

No nó contraído, os objetos da mochila são grupos inteiros, e `u_G` é o dual da igualdade daquele grupo. Não se descartam itens individuais de dentro de um grupo.

Grupos de dual não positivo podem ser ignorados: retirar um grupo de um padrão preserva capacidade e ausência de conflitos e não diminui seu valor. Se nenhum grupo tem valor positivo, não existe coluna de soma dual maior que 1.

### Pricing heurístico

O código tenta 12 ordens de grupos: densidade `u_G/s(G)`, valor `u_G` e embaralhamentos. Para cada ordem, faz uma seleção gulosa respeitando capacidade e conflitos. Adiciona colunas novas com valor maior que `1+1e-7`.

Se a heurística não encontrar uma coluna, isso significa apenas que suas tentativas falharam. Não prova que não existe uma.

### Pricing exato

Somente quando a heurística não adiciona colunas, o programa chama `Pricing::exact`. A enumeração considera incluir ou excluir cada grupo de dual positivo.

Para podar a enumeração, calcula a mochila **fracionária sem conflitos** dos grupos ainda disponíveis. Permitir frações e ignorar conflitos amplia as possibilidades; portanto o valor resultante é um limite **superior** para o pricing. Se nem esse valor pode superar a melhor seleção já encontrada, aquele ramo pode ser abandonado.

Ordenar por densidade permite calcular essa cota enchendo a capacidade com grupos inteiros e uma fração do próximo. A seleção realmente devolvida pelo pricing continua inteira e respeita todos os conflitos.

Se a enumeração termina, `best` aproxima o máximo do pricing, consideradas as tolerâncias. Se o prazo acaba, `best` é apenas um valor viável, logo um limite **inferior** para o máximo da mochila. Nesse caso, o programa conserva em `upper` a cota superior da mochila fracionária inicial.

Confundir `best` com uma cota superior quando há interrupção é um erro que pode produzir um falso certificado de otimalidade.

## 7. De onde vem o limitante de Farley?

Suponha que conhecemos `alpha >= 1` com:

\[
\sum_{i\in B}u_i\le\alpha\quad\text{para todo padrão válido }B.
\]

Então o vetor `u/alpha` satisfaz todas as restrições do dual completo. Por dualidade fraca:

\[
z_{LP}\ge\frac{\sum_i u_i}{\alpha}.
\]

Se os duais são ótimos para o PMR, a soma é seu objetivo, dando a forma usual `z_PMR/alpha`.

Exemplo: `z_PMR=6` e uma cota superior de pricing `alpha=1,2` dão `z_LP >= 5`. Não seria correto usar 6 como LB diretamente. Se a busca exata certificar `alpha*=1`, temos `z_PMR=z_LP`.

Na implementação, `certifiedBound` usa a soma dos duais e uma cota superior do pricing. Como o objetivo inteiro é uma contagem, arredonda-se para cima com tolerância:

\[
LB_{nó}=\max\left\{LB_{pai},\left\lceil\frac{\sum_G u_G}{\alpha}-10^{-6}\right\rceil\right\}.
\]

O denominador também recebe uma margem conservadora. Farley pode justificar a poda antes de uma enumeração exata completa, desde que `alpha` seja uma cota superior válida. A cota fracionária sem conflitos já fornece isso; o pricing exato pode fortalecê-la.

## 8. Por que precisamos de Ryan–Foster?

A solução da relaxação pode usar frações de recipientes. Para chegar a uma solução inteira, criamos subproblemas que particionam as soluções possíveis.

Para dois grupos distintos `a,b`, calcula-se:

\[
\phi_{ab}=\sum_{B\supseteq\{a,b\}}x_B.
\]

Se `0 < phi_ab < 1`, os grupos estão parcialmente juntos na solução fracionária. O código escolhe um par com valor próximo de `1/2` e cria dois filhos:

- **Juntos:** ambos devem aparecer no mesmo recipiente; contraem-se em um grupo.
- **Separados:** ambos não podem aparecer no mesmo recipiente; acrescenta-se um conflito.

Toda solução inteira coloca esse par junto ou separado, exatamente uma das alternativas. Assim, os dois filhos cobrem todas as soluções inteiras do pai.

### Como as decisões chegam ao pricing

`Groups` usa union-find para calcular as componentes das decisões juntos. Exemplo: juntar `(1,3)` e depois `(3,5)` cria o grupo `{1,3,5}`.

O tamanho do grupo é a soma dos tamanhos originais. Um grupo é inválido se ultrapassa a capacidade ou contém um conflito original ou uma decisão separados. Grupos distintos conflitam se qualquer par de seus itens conflita.

O pricing escolhe grupos inteiros. Uma coluna global reaproveitada precisa conter todos os itens de cada grupo ou nenhum deles. Por exemplo, depois de juntar `1` e `3`, a coluna `{1,4}` é proibida; `{1,3,4}` só pode ser usada se respeitar capacidade e conflitos.

Essa propagação é essencial: remover uma coluna do PMR sem impedir que o pricing a gere de novo não implementa corretamente a ramificação.

## 9. Podas e limite global

Um nó pode ser encerrado quando:

1. Seus grupos revelam inviabilidade.
2. Seu limitante inferior inteiro é pelo menos a incumbente global.
3. Após o pricing ser certificado, a solução do mestre é inteira e encerra o subproblema, dentro das tolerâncias.

No terceiro caso, a solução é expandida para itens originais e verificada antes de atualizar a incumbente.

A fila prioriza o menor limitante conhecido, com desempate pela criação. Os filhos herdam o LB do pai: adicionar restrições não pode diminuir o ótimo do subproblema.

Enquanto um nó é processado, ele permanece representado na fila. Só é removido se encerrado ou substituído pelos **dois filhos**. Caso o tempo acabe, sua subárvore continua contribuindo para a cota global.

Se existem nós abertos:

\[
LB_{global}=\min\{UB,\min_{N\text{ aberto}}LB_N\}.
\]

O código preserva também a cota inicial. Se a fila esvaziou, `LB_global=UB`.

Não se deve usar a cota do último nó visitado ou a maior cota da fila como limite global: pode existir outra subárvore ainda aberta com ótimo menor.

## 10. Exemplo do enunciado: provar quatro à mão

Os oito tamanhos são `5,5,4,4,3,3,3,3`, capacidade 10 e classes `{1,2}`, `{3,4}`, `{5,6,7}`. O volume total é 30 e a maior classe tem três itens. Portanto `LB_0=3`.

Uma solução de quatro recipientes:

| Recipiente | Itens | Carga |
|---|---|---|
| 1 | 1, 3 | 9 |
| 2 | 2, 5 | 8 |
| 3 | 4, 6, 8 | 10 |
| 4 | 7 | 3 |

Para provar que três são impossíveis: três recipientes de capacidade 10 precisariam estar todos cheios. O item 1, de tamanho 5, precisaria de outros itens somando exatamente 5. O item 2 é proibido com ele. Os demais tamanhos são 4 ou 3, e nenhuma combinação soma 5. Logo `OPT >= 4`. A solução da tabela prova `OPT <= 4`, fechando a prova.

Esse argumento é independente do solver e explica por que o limitante trivial três ainda não basta.

## 11. Exemplo que força ramificação

A instância `testes/instancias/ryan_foster.txt` tem dez itens de tamanho 1 e capacidade 10. Os cinco primeiros formam um ciclo de conflitos `1-2-3-4-5-1`; os cinco últimos formam outro ciclo. Todo item do primeiro ciclo conflita com todo item do segundo.

Um recipiente não mistura os ciclos e contém no máximo dois itens, pois o maior conjunto independente de um ciclo de cinco vértices tem tamanho 2.

Cada ciclo exige pelo menos três recipientes: dois recipientes cobrem no máximo quatro itens. Três bastam, por exemplo `{1,3}`, `{2,4}`, `{5}`. Como não se misturam os ciclos, `OPT=6`.

A relaxação admite valor cinco: em cada ciclo existem cinco pares independentes, e cada vértice pertence a dois deles. Usar cada par com peso `1/2` cobre cada item uma vez, com custo `5/2` por ciclo. O dual `u_i=1/2` também é viável, pois todo padrão tem no máximo dois itens. Logo `z_LP=5`.

Portanto a igualdade `ceil(z_LP)=OPT` não vale nesta instância. A árvore precisa continuar.

```bash
make run INPUT=../testes/instancias/ryan_foster.txt OUTPUT=../saida-rf.txt TIME=30 SEED=1
python3 testes/conferir_solucao.py -f testes/instancias/ryan_foster.txt -o saida-rf.txt --exato
```

Na instalação validada, o log tem `nodes=3`, `branches=1`, `LB=6`, `UB=6`. Isso significa processar a raiz e os dois filhos. Os contadores podem mudar com versão/bases do solver; a propriedade a preservar é o ótimo seis e a validade das decisões.

Para visualizar a decisão, considere o par compatível `{1,3}`. Juntá-lo cria o grupo `{1,3}`; separá-lo adiciona a aresta de conflito `1-3`. Cada filho mantém todas as demais restrições. O par efetivamente escolhido depende da solução linear encontrada.

## 12. O que a suíte realmente verifica

`testes/verificar.py` implementa um algoritmo diferente: enumera subconjuntos válidos e resolve uma programação dinâmica de partição.

Se `M` é o conjunto de itens restantes e `i` é seu menor item:

\[
DP(\varnothing)=0,
\qquad DP(M)=1+\min_{\substack{B\subseteq M,\ i\in B\\B\text{ válido}}}DP(M\setminus B).
\]

Toda partição contém exatamente um recipiente com `i`. Examinar todas essas escolhas e resolver o restante considera todas as partições possíveis. Fixar `i` evita repetir escolhas apenas por permutar a ordem dos recipientes. A prova de correção é uma indução no número de itens de `M`.

Esse oráculo não usa Gurobi, duais, geração de colunas, grupos nem decisões Ryan–Foster. Por isso ajuda a detectar erros nesses componentes sem reproduzir sua implementação.

A suíte padrão compara `LB <= OPT_oráculo <= UB` e exige `LB=UB=OPT_oráculo` para 187 instâncias pequenas: sete fixas, 120 com tamanhos/classes aleatórios e 60 de grafos. A semente do gerador de instâncias é `20261111`.

Também foi executada a rodada ampliada com `--random 500`: **567 instâncias**,
todas com `LB=UB=OPT_oráculo`, 255 chamadas de pricing exato e uma ramificação.
O registro está em `testes/resultados-ampliados.json`. Essa ampliação adiciona
variedade de entradas; a cobertura de árvores profundas continua limitada.

Além disso, testa interrupções em um ciclo ímpar de 31 itens, cujo ótimo é três; os testes aceitam qualquer saída viável com `LB <= 3 <= UB`. Também verifica números inválidos, repetição de itens em classes, classes vazias, parâmetros da CLI e capacidade de 64 bits.

Limites dessas evidências:

- Os casos pequenos não esgotam todas as entradas possíveis.
- O teste temporal aceita uma folga de um segundo sobre o tempo solicitado; ele não prova cumprimento rígido de T.
- Os contadores mostram execução do pricing exato e da ramificação, mas não cobrem todos os caminhos internos.
- Não há uma comparação isolada de cada chamada de pricing com outro solver.
- Sanitizers verificam erros durante as execuções feitas; ausência de alertas não demonstra correção matemática.

O código usa PL em ponto flutuante, tolerâncias e cotas com margens; não produz um certificado racional verificável de todos os nós. Confiança vem da combinação de argumento matemático, revisão do código e verificação independente dos resultados.

## 13. Roteiro para reproduzir a conferência

### A. Reconstruir e testar

```bash
make clean
make
make test
python3 testes/verificar.py --random 500 --report /tmp/bnp-validacao-ampliada.json
```

O último comando aumenta os casos com tamanhos/classes para 500; mantém os sete fixos e os 60 grafos. Não execute com `python3 -O`: os testes usam `assert`.

### B. Conferir qualquer saída pequena

```bash
python3 testes/conferir_solucao.py -f testes/instancias/exemplo.txt -o saida.txt --exato
```

O modo exato aceita até 16 itens e pode custar tempo exponencial. Sem `--exato`, o comando confere apenas partição, capacidade, conflitos e coerência dos números. Ele não prova o LB informado.

Uma saída pode ser viável, porém ainda não ótima. O verificador informa o valor ótimo independente quando solicitado e a diferença para a solução, sem confundir viabilidade com otimalidade. Um LB maior que o ótimo independente é rejeitado.

### C. Variar sementes e orçamento

```bash
make run TIME=0 OUTPUT=../saida-zero.txt
make run TIME=1 SEED=42 OUTPUT=../saida-semente42.txt
```

Com `T=0`, o exemplo devolve oito unitários e LB três. A solução é viável, mas não ótima. Como há aleatoriedade e cortes por relógio, execuções com sementes/orçamentos distintos podem encontrar caminhos diferentes.

Compare valores e validade. Exigir a mesma ordem textual de recipientes não é um bom teste.

### D. Detectar erros de memória e comportamento indefinido

```bash
make clean
make CXXFLAGS='-O1 -g -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer'
make test
make clean
make
```

O Makefile da raiz encaminha as variáveis para `src/`. No ZIP extraído, acrescente `-C codigo` aos comandos `make`.

### E. Conferir os invariantes mais importantes no código

| Invariante | Onde procurar | Erro que previne |
|---|---|---|
| Incumbente sempre é uma partição viável | `SolveBinPacking`, `FirstFitRound`, `CheckSolution` | Gravar solução parcial ou conflitante |
| Só variáveis contínuas no mestre | lambda `add`, `GRB_CONTINUOUS` | Delegar a árvore principal ao MIP |
| Coluna nova tem custo reduzido negativo | pricing heurístico e teste de `pricing.best` | Crescer o PMR sem critério |
| Heurística vem antes do pricing exato | laço de geração, `if(added) continue` | Desrespeitar o fluxo exigido |
| Toda cota de Farley usa um limite superior de pricing | `fractional`, `exact`, `certifiedBound` | Podar usando um máximo não provado |
| Decisões são herdadas e afetam todo padrão | `Groups`, `admissible`, cópia dos filhos | Regenerar coluna proibida |
| Padrões não se repetem no nó | conjunto `local` | Duplicação e problemas na escolha do par |
| O nó atual não desaparece ao interromper | manipulação de `open` | Aumentar LB global indevidamente |

## 14. Mapa para ler os fontes

| Arquivo | Símbolos principais | Pergunta para orientar a leitura |
|---|---|---|
| `src/main.cpp` | `ReadInstance`, `main`, `WriteSolution` | Como a entrada vira dados e como a saída é validada? |
| `src/binpacking.h` | `Prepare`, `Conflict`, `TrivialLowerBound`, `CheckSolution` | Que propriedades são válidas antes da busca? |
| `src/binpacksolver.cpp` | `ItemPriority`, `FirstFitRound`, `RandomizedFFD`, `SolveBinPacking` | Como obter uma incumbente e iniciar o relógio? |
| `src/branchprice.cpp` | `Node`, `Groups`, `Pricing`, `certifiedBound`, `BranchAndPrice` | Como o problema completo é resolvido sem listar todas as colunas? |
| `testes/verificar.py` | `oracle`, `check_output` | Como conferir por um método independente? |
| `testes/conferir_solucao.py` | `main` | Como verificar um arquivo produzido numa execução manual? |
| `testes/empacotar.py` | `export`, `package` | Como registrar a conversa e produzir a estrutura exigida? |

No primeiro estudo, siga essa ordem. Dentro de `branchprice.cpp`, leia primeiro o laço principal, depois `Groups`, depois o pricing e finalmente a manipulação da fronteira. Use os nomes das funções para localizar os trechos; números de linha mudam quando o código é editado.

## 15. Perguntas para testar sua compreensão

Tente responder antes de ler as respostas.

1. Se o PMR contém só unitários e vale 20, podemos concluir `OPT >= 20`?
2. Por que os duais das restrições de cobertura podem ser negativos?
3. Se o pricing guloso falha, por que chamar o exato?
4. O pricing interrompido encontrou valor 1,4. Podemos usar `alpha=1,4` em Farley?
5. Qual a diferença entre a poda da enumeração do pricing e a poda de um nó da árvore principal?
6. Depois de juntar `(1,3)` e `(3,5)`, a coluna `{1,3}` é admissível?
7. A melhor cota de algum nó aberto é oito e a menor é cinco. Qual delas limita o ótimo global?
8. Por que o exemplo com dois ciclos exige ramificação mesmo com o pricing certificado?
9. Se a heurística inicial obtém `UB=LB_0`, precisamos chamar Gurobi?
10. Qual a diferença entre `LB=UB` no log e a confirmação do oráculo independente?

### Respostas comentadas

1. Não. Faltam padrões; o mestre restrito pode superestimar até o ótimo inteiro. É necessário certificar o pricing ou aplicar uma cota válida como Farley.
2. Porque o primal usa igualdades. A liberdade de sinal vem da dualização; o pricing pode ignorar grupos não positivos pelas propriedades de capacidade/conflitos, sem alterar a formulação dual.
3. A heurística não examina todas as possibilidades. Só uma prova de ausência de coluna melhorante ou uma cota superior certificada permite tirar conclusões para o LB.
4. Não apenas com essa informação. 1,4 é um valor atingido, portanto está abaixo ou igual ao máximo. Farley requer uma cota que esteja acima ou igual a todos os valores possíveis.
5. O pricing é maximização: uma cota superior pequena permite descartar uma seleção parcial. A árvore principal é minimização: uma cota inferior pelo menos igual à incumbente permite descartar um nó.
6. Não. O grupo é `{1,3,5}` e deve entrar inteiro ou não entrar.
7. A menor, limitada também por UB. A subárvore de cota cinco ainda pode conter uma solução melhor que oito.
8. Seu LP vale cinco, mas nenhuma solução inteira usa menos de seis recipientes. Certificar o LP não elimina a lacuna de integralidade.
9. Não. Já existe uma solução com o mesmo valor de uma cota válida; logo é ótima.
10. O log informa a conclusão do algoritmo que estamos verificando. O oráculo recalcula OPT por outro método e confirma o valor em uma instância pequena. Nenhum teste finito substitui toda a justificativa matemática da implementação.

## 16. Limitações para explicar na apresentação

O método é exato no modelo combinatório, com implementação numérica sujeita às tolerâncias do PL e do pricing. A enumeração do pricing e a árvore são exponenciais no pior caso. A matriz de conflitos dos grupos ocupa memória quadrática; o banco de colunas pode crescer. Não há estabilização dual nem heurística MIP opcional.

O tempo usa `steady_clock`, limite restante no Gurobi e callback, mas inicialização de licença, chamadas nativas e escalonamento podem ultrapassar o prazo. Os testes atuais não provam o requisito literal de término rígido em T; essa é uma limitação explícita para uma avaliação estrita do enunciado.

Uma explicação oral suficiente deve conectar as peças: padrão válido → partição → relaxação → dual → pricing → cota → poda/ramificação → fronteira ao interromper. Se conseguir justificar cada seta e executar os exemplos acima, você terá uma base para defender e modificar o trabalho.
