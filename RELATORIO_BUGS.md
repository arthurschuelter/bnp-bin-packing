# Relatório de problemas do código (`src/`)

Revisão feita em 19/11/2026 sobre o commit `371106c`, comparando o código com
`enunciado.pdf` e `projeto-template/`. Na data da revisão nenhuma correção
tinha sido aplicada; o texto de cada seção descreve o problema como foi
encontrado e a sugestão de solução.

**Status atualizado em 21/11/2026** (commit `41d35f3`): os itens 1, 2, 5 e 6
foram resolvidos e o 4 e o 10 foram resolvidos em parte; os demais continuam abertos.
A coluna "Status" da tabela e a linha **Status** no início de cada seção
registram isso. Os números de linha citados referem-se ao commit `371106c`
e não correspondem mais ao código atual.

## Resumo

A **corretude** da implementação está boa. A solução gravada é sempre viável,
o limitante inferior está certificado (Farley + pricing exato) e as decisões
de Ryan–Foster estão modeladas corretamente com grupos. Os problemas reais estão
no **desempenho da árvore**, na **robustez** e na **configuração e documentação**.

| # | Severidade | Problema | Onde (em `371106c`) | Status |
|---|---|---|---|---|
| 1 | Alta | A árvore é percorrida praticamente em largura e não tem heurística primal. O ótimo não é encontrado mesmo com o LB já correto. | `branchprice.cpp:37-43`, `:467-475` | Resolvido |
| 2 | Média | Uma falha em um único nó (status do LP diferente de ótimo, exceção numérica) encerra todo o B&P. | `branchprice.cpp:378-382`, `:413`, `:466` | Resolvido |
| 3 | Média | O orçamento da heurística FFD pode impedir até uma rodada completa, deixando só a solução unitária. | `binpacksolver.cpp:149-155` | Aberto |
| 4 | Média | O modelo é reconstruído do zero em cada nó e o pool inteiro é varrido; o método simplex escolhido é inadequado para geração de colunas. | `branchprice.cpp:330-368` | Parcial |
| 5 | Média | Um `make` simples falha no Linux, porque o `GUROBI_HOME` padrão é o caminho do macOS. | `src/Makefile:4`, `src/compile_flags.txt` | Resolvido |
| 6 | Baixa | Código e documentação divergem: os logs do Gurobi estão desligados, mas o README diz que estão ligados. | `branchprice.cpp:306-307`, `README:49` | Resolvido |
| 7 | Baixa | A FFD usa memória O(k) por recipiente. | `binpacksolver.cpp:94` | Aberto |
| 8 | Baixa | `-t` perto do máximo do `steady_clock` pode estourar o cálculo do prazo. | `commandline.cpp:36-44`, `binpacksolver.cpp:146` | Aberto |
| 9 | Baixa | Ineficiências do pricing (uma coluna por chamada exata, relógio consultado em todo nó da DFS). | `branchprice.cpp:186-224`, `:82-84` | Aberto |
| 10 | Baixa | Higiene do código e avisos do lint. | vários | Parcial |

---

## 1. Busca em largura sem heurística primal (ALTA)

**Status (21/11/2026):** Resolvido. `Node` ganhou `depth` e o comparador `BestBound` desempata pelo nó mais profundo (commit `bff883e`). A heurística MIP de arredondamento `roundingHeuristics` roda no nó raiz e a cada 50 nós, depois da convergência da geração de colunas (commit `a9ce601`; README, item 9).

**Problema.** A fila de prioridade ordena por `bound` e, em caso de empate, por
`id` crescente (`BestBound`, `branchprice.cpp:37-43`). Como os limitantes são
inteiros (`ceil`), quase todos os nós empatam. Na prática, então, a ordem é
**FIFO, isto é, busca em largura**. Uma busca em largura quase nunca chega a uma
folha inteira, e o código só melhora a incumbente quando o LP de um nó convergido
sai inteiro (`:435-447`). Não há arredondamento nem um MIP sobre as colunas
geradas, embora a Seção 10.6 do enunciado permita isso.

**Evidência.** Testei a instância abaixo (gerada aleatoriamente: n = 60,
C = 150, k = 10):

```
60 150 10
38 66 34 46 37 61 58 60 71 54 43 36 61 31 54 57 68 30 74 58 47 44 67 36 50 31 31 31 71 64 30 54 73 43 57 31 63 44 58 61 65 44 52 44 73 44 59 48 31 56 65 71 36 41 70 48 37 51 75 62
5 33 54 59 43 13
4 19 38 57 32
6 26 38 55 3 31 16
5 27 43 12 24 36
4 6 29 43 33
2 50 11
6 54 26 24 32 47 2
5 3 20 46 55 40
6 38 26 42 11 57 33
3 1 50 13
```

```
-t 10 : [B&P] nodes=2821  branches=2820  LB=21 UB=22
-t 60 : [B&P] nodes=14026 branches=14025 LB=21 UB=22
```

Uma formulação compacta independente (atribuição item×recipiente resolvida pelo
`gurobi_cl`) achou uma solução com **21 recipientes**, então **OPT = 21**. O LB da
raiz já era ótimo; faltou apenas achar a solução. Em 60 s o programa processou
14 mil nós e nunca melhorou a incumbente da FFD. O mesmo padrão (LB = UB − 1,
milhares de nós) apareceu em outra instância com n = 100. A suíte `make test`
não detecta isso, porque as instâncias dela são pequenas.

**Como resolver** (em ordem de custo-benefício):

1. **Desempate por profundidade (mergulho).** Guarde `depth` em `Node` e, entre
   nós com o mesmo `bound`, prefira o mais profundo. Assim se obtém uma busca
   "melhor limitante + DFS":
   ```cpp
   struct Node { ...; int depth = 0; };
   return a.bound != b.bound ? a.bound > b.bound
        : a.depth != b.depth ? a.depth < b.depth   // mais profundo primeiro
        : a.id > b.id;
   ```
   Empilhe o filho "juntos" por último (ou dê a ele um `id` menor), porque ele
   costuma levar mais rápido a soluções inteiras.
2. **Heurística primal no nó.** Depois que a geração de colunas converge (ou a
   cada N nós), resolva o PMR com as variáveis **binárias** e com limite de tempo
   curto (permitido pela Seção 10.6). Outra opção mais barata é um arredondamento
   guloso: escolha as colunas com maior `x`, remova os itens já cobertos e complete
   com FFD. Passe o resultado por `CheckSolution` e depois chame `UpdateBest`.
3. Quando houver uma incumbente melhor, a poda `open.top().bound >= best_value`
   já descarta o resto da árvore. Neste exemplo, a execução terminaria com
   LB = UB = 21.

## 2. Uma falha local aborta todo o B&P (MÉDIA)

**Status (21/11/2026):** Resolvido. Cada nó tem `try/catch` próprio. Uma falha local (`GRBException`, `NodeFailure`) estaciona o nó na lista `parked` com sua cota certificada e a busca continua. O LB global é o mínimo sobre `open` e `parked`. Status do PL: `GRB_TIME_LIMIT`/`GRB_INTERRUPTED` param tudo; os demais geram uma nova tentativa com `reset`, `NumericFocus=3` e `Method=1`. Coluna repetida: o nó é tratado como convergido dentro da tolerância (Farley vale para qualquer dual); a solução inteira só fecha o nó se a cota alcançar a incumbente. Solução inteira inviável: ramifica. Sem par fracionário acima de `INT_EPS`: usa o par menos integral; se não houver, estaciona. O resumo `[B&P]` mostra `failed=`. Teste: `BNP_FAULT_NODE=k` simula a falha; `verificar.py` cobre k = 1, 2 e 3 em `ryan_foster` (a busca continua, a saída é viável, LB < UB). Sem regressão em `make test` e no estresse de 60 instâncias com ramificação.

**Problema.** Três situações terminam a busca inteira, e não só o nó atual:

- `model.get(GRB_IntAttr_Status) != GRB_OPTIMAL` → `stopped = true` → `break`
  do laço externo (`:378-382`, `:489-490`). Esse ramo existe para o caso de tempo
  esgotado. Mas status como `NUMERIC` ou `SUBOPTIMAL` também caem nele, e então o
  programa para mesmo com muito tempo sobrando. (O comentário
  `// Por que != de GRB_OPTIMAL` na linha 377 parece justamente essa dúvida.)
- `throw std::runtime_error("pricing repeated ...")` (`:413`) e
  `"no fractional Ryan-Foster pair"` (`:466`): ambos são capturados em `:502` e
  encerram o B&P.

O LB devolvido continua válido, porque o nó atual permanece na fronteira. O que
se perde é todo o tempo restante.

**Como resolver.**
- Diferencie os status: só `GRB_TIME_LIMIT` e `GRB_INTERRUPTED` significam "parar
  tudo". Para os outros, tente de novo com `Method` diferente ou
  `NumericFocus = 2`. Se ainda falhar, deixe esse nó de lado (devolvendo-o à
  fronteira com o bound herdado, o que mantém o LB válido) e continue com os
  próximos.
- Para "coluna repetida" e "nenhum par fracionário": trate como tolerância
  numérica. Considere o nó convergido. Se o `x` estiver quase inteiro,
  arredonde e verifique com `CheckSolution`. Caso contrário, escolha o par com
  `φ` mais distante de 0/1 sem exigir `INT_EPS`.

## 3. Orçamento da FFD pode deixar só a solução unitária (MÉDIA)

**Status (21/11/2026):** Aberto. `RandomizedFFD` ainda para quando `elapsed >= time_limit` já na primeira rodada, e `FirstFitRound` recebe o prazo de 5%, não o global.

**Problema.** `RandomizedFFD(data, 1000, min(0.25, time_limit * 0.05), start)`
(`binpacksolver.cpp:154`). O template sempre executava **ao menos uma rodada**
(`round > 0 && elapsed >= time_limit`). A versão atual não tem essa garantia.
Além disso, `FirstFitRound` interrompe e descarta uma rodada parcial quando o
prazo de 5% acaba. Para T pequeno (por exemplo `-t 1` → 0,05 s) ou n grande, a
incumbente inicial pode ser apenas `{1},{2},...,{n}`. O enunciado pede que essa
heurística seja a fonte da incumbente inicial e das colunas iniciais. Além
disso, esse comportamento não segue o critério de parada descrito ("até 1000
rodadas, ou até T, ou até atingir o LB").

**Como resolver.** Garanta ao menos uma rodada completa, limitada só pelo prazo
global `deadline`. O orçamento de 5% deve valer apenas a partir da segunda
rodada:
```cpp
if (round > 0 && elapsed >= heuristic_budget) break;
// e passar o deadline GLOBAL para FirstFitRound
```

## 4. Reconstrução do modelo a cada nó e escolha do simplex (MÉDIA, desempenho)

**Status (21/11/2026):** Parcial. O mestre usa `Method = 0` (primal simplex). Cada filho herda as colunas do mestre final do pai (`Node::columns`), filtradas só pelo par novo em `inheritColumns`; só a raiz varre o banco global. Continua aberto: o `GRBModel` ainda é recriado em cada nó. Medição: medium-3 com `-t 30` provou o ótimo em 2,6 s e 1,4 s (sementes 1 e 2), contra 4,4 s e 2,2 s antes. Na raiz de hard-1, com `-t 5`, foram 619 PLs contra 533. Em 60 instâncias de até 16 itens geradas para ramificar (119 ramificações), todas as saídas tiveram UB = LB = ótimo do oráculo independente.

**Problema.**
- Cada nó cria um `GRBModel` novo, adiciona n linhas e varre **todo** o
  `pool` (cópia em `snapshot` + `admissible`, que custa O(nº de grupos) por
  padrão). O custo por nó é O(|pool|·n) e cresce com a árvore.
- `GRB_IntParam_Method = 1` (dual simplex) com o comentário "incremental
  columns". Na geração de colunas, acrescentar colunas mantém a base **primal**
  viável e destrói a viabilidade dual. Por isso, o **primal simplex
  (`Method = 0`)** é o que aproveita a base entre as iterações.

**Como resolver.**
- Use `Method = 0` dentro do laço de geração de colunas.
- Mantenha um só modelo por "mergulho": no filho, em vez de recriar o modelo,
  fixe em `ub = 0` as colunas incompatíveis com a nova decisão. Outra opção mais
  simples é limitar o `snapshot` às colunas com custo reduzido pequeno no pai.
- Pré-aloque `rows`/`vars` (`reserve`), como o lint sugere.

## 5. `make` padrão falha no Linux (MÉDIA, ambiente)

**Status (21/11/2026):** Resolvido. `src/Makefile` detecta o sistema por `uname` e escolhe a instalação mais recente em `/opt/gurobi*/linux64` ou `/Library/gurobi*/macos_universal2`. O nome da biblioteca é deduzido de `$(GUROBI_HOME)/lib`.

**Problema.** `src/Makefile:4` define
`GUROBI_HOME ?= /Library/gurobi1302/macos_universal2` (caminho do Mac). Neste
computador, o Gurobi está em `/opt/gurobi1303/linux64`, então um `make` sem
argumentos não compila. Já `src/compile_flags.txt` aponta para o Linux, mas está
no `.gitignore`. As configurações do editor e da compilação divergem.

**Como resolver.** Detecte o sistema (ou use a variável de ambiente que o
instalador do Gurobi define):
```make
ifeq ($(shell uname),Darwin)
GUROBI_HOME ?= /Library/gurobi1302/macos_universal2
else
GUROBI_HOME ?= /opt/gurobi1303/linux64
endif
```
Atualize também `AGENTS.md` e `CONTEXTO_CODEX.md`, que dizem que os caminhos
"foram configurados para um Mac".

## 6. Logs do Gurobi: código ≠ documentação (BAIXA)

**Status (21/11/2026):** Resolvido. `OutputFlag = 1`, como dizem o README e o contexto. A linha comentada com `OutputFlag = 0` continua no código como alternativa.

`branchprice.cpp:306` usa `OutputFlag = 0` (a linha com `1` está comentada).
`README:49` e `docs/CONTEXTO_CODEX.md:99` afirmam que os logs estão ligados
(`OutputFlag=1`), um pedido explícito anterior. Escolha um dos dois e alinhe.
Uma sugestão é uma flag `-v` na CLI ou uma variável de ambiente, e remover a
linha comentada.

## 7. Memória da FFD com muitas classes (BAIXA)

**Status (21/11/2026):** Aberto. A FFD ainda aloca `vector<char>(data.k)` por recipiente.

`used.push_back(vector<char>(data.k, 0))` (`binpacksolver.cpp:94`) aloca k bytes
**por recipiente aberto**, em toda rodada. Em instâncias que codificam o grafo
com uma classe por aresta (k grande), o custo fica em O(k·m) de memória e tempo
por rodada. Por exemplo, k = 30 000 com ~1 500 recipientes dá ≈ 45 MB por rodada.
Isso também aumenta a chance do problema 3.

**Como resolver.** Guarde, por recipiente, os itens dele. Teste o conflito com
um vetor `mark[t]` reutilizado (carimbo com o índice do recipiente), ou mantenha
`std::vector<int>` ordenado com as classes do recipiente.

## 8. Estouro no cálculo do prazo com `-t` enorme (BAIXA)

**Status (21/11/2026):** Aberto. Não há teto para T nem saturação do prazo.

`ParseCommandLine` aceita `T < max_seconds`, calculado com o `now()` **do
parse**. Depois, `SolveBinPacking` calcula
`start + duration_cast<steady_clock::duration>(duration<double>(T))` com um
`start` posterior e com conversão de `double`, que pode arredondar para cima.
Para T muito próximo do limite (≈ 9,2·10⁹ s menos o uptime), a soma estoura
`int64`, o que é comportamento indefinido. **Correção:** limite T a um teto
generoso, como `1e9` s, ou calcule o prazo uma única vez, saturando em
`time_point::max()`.

## 9. Ineficiências no pricing (BAIXA)

**Status (21/11/2026):** Aberto.

- `Pricing::dfs` chama `Clock::now()` em **todo** nó da enumeração, e
  `Groups::forbid` chama `checkTime` para cada par de cada classe (O(Σ|S_t|²)
  chamadas de relógio por nó). Consulte o relógio a cada ~1024 nós.
- O pricing exato devolve **uma** coluna por chamada. Guardar as *k* melhores
  colunas com `u(B) > 1` (ou todas acima do limiar encontradas na DFS) reduz o
  número de chamadas exatas.
- O limite de Farley é calculado com a mochila fracionária **sem conflitos**.
  Ela é válida, mas fraca. Depois de uma chamada exata completa, já se usa
  `best`, e isso está correto.

## 10. Higiene e lint (BAIXA)

**Status (21/11/2026):** Parcial. `#include <chrono>` foi adicionado, `HasSolution()` passou a ser usada em `main.cpp`, e o comentário de dúvida saiu do código. Continuam: `rng` estático com semente constante, `TimeExpired` sem base `std::exception`, `catch` vazio e falta de `reserve`. Em 21/11/2026, `make lint` mostrou 6 avisos, entre eles um novo em `binpacking.h:46` (campo não inicializado no construtor).

`make format-check` passa. `make lint` mostra 5 avisos:
- `TimeExpired` não deriva de `std::exception`; `catch (const TimeExpired &) {}`
  está vazio. Faça a classe herdar de `std::exception` e deixe um comentário ou
  log no `catch`.
- `static mt19937_64 rng(1)` global: mova para dentro de `SolveBinPacking`.
- `push_back` em laço sem `reserve` (`branchprice.cpp:337`).

Outros pontos:
- `binpacksolver.cpp` usa `std::chrono` sem `#include <chrono>`. Hoje compila
  só porque `branchprice.h` inclui esse header.
- `HasSolution()` não é usada. `maxcls` é recalculado a cada rodada da FFD.
- As mensagens de erro misturam `cout` e `cerr` (por exemplo, "Invalid time
  limit" vai para `cerr` sem o uso; erros de leitura vão para `cout`).
- Há um comentário de dúvida deixado no código (`branchprice.cpp:377`).

---

## O que foi verificado

- `make test GUROBI_HOME=/opt/gurobi1303/linux64`: **OK**, 187 ótimos
  certificados.
- `make format-check`: OK. `make lint`: 5 avisos, nenhum erro.
- Instâncias aleatórias maiores (n = 60 a 3 000) com `-t 5/10/60`: todas as
  soluções eram viáveis, o tempo limite foi respeitado (≤ T + ms) e os LBs eram
  válidos. Nas instâncias médias apareceu o problema 1.
- OPT = 21 da instância do problema 1 foi confirmado por um MIP compacto
  independente (`gurobi_cl`).

Não encontrei nenhum caso de solução inviável ou de LB > OPT.
