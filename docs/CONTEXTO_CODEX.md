# Contexto para continuar com Codex em outro computador

Registro preparado em 11/11/2026 e atualizado em 21/11/2026. Leia este arquivo
primeiro, depois o README e os trechos relevantes do guia de estudo. Consulte a transcrição para
recuperar detalhes que não cabem neste resumo.

## Objetivo do usuário

Desenvolver o Projeto 2 de Programação Linear Inteira descrito em
`enunciado.pdf`, usando o código e as instâncias fornecidos como base.
O problema é Bin Packing com Classes de Conflito. O usuário também quer
entender, verificar, estudar e padronizar o trabalho.

O projeto já está implementado e, em 21/11/2026, estava em revisão final para
a entrega. O trabalho continuou em Linux (CachyOS), com Codex e Claude Code.
Não existe uma nova tarefa de implementar o algoritmo desde o início.

## O que este registro transfere

- Os arquivos atuais do projeto, inclusive alterações ainda não commitadas.
- As decisões técnicas, verificações e pendências descritas aqui.
- Os prompts, respostas e ferramentas disponíveis em `ai/conversa-AAAA-MM-DD.md`
  e `ai/conversa-AAAA-MM-DD.jsonl`, em ordem cronológica.
- Configurações de formatação, lint e VS Code.

Não transfere uma instância de modelo em execução, raciocínio interno,
ferramentas autenticadas, licença do Gurobi nem o histórico/índice do Git.
O ZIP é um snapshot dos arquivos atuais, não um backup de `.git`.
A transcrição exportada é material de leitura para um novo chat; não é
um arquivo garantido de importação de sessão do aplicativo.

## Estrutura atual

| Caminho | Função |
|---|---|
| `enunciado.pdf` | Requisitos oficiais e regras da entrega |
| `projeto-template/` | Código original; preservar |
| `src/` | Implementação C++ ativa e Makefile |
| `testes/instancias/` | Exemplo e casos fixos adicionais |
| `testes/verificar.py` | Suíte com oráculo exato independente por subconjuntos |
| `testes/conferir_solucao.py` | Conferência de uma saída; `--exato` até 16 itens |
| `docs/GUIA_DE_ESTUDO.md` | Algoritmo, provas, exemplos, experimentos e exercícios |
| `docs/LINT_E_FORMATACAO.md` | Uso e regras das ferramentas de lint/formatação |
| `ai/` | Conversa exportada e estado da exportação |
| `RELATORIO_BUGS.md` | Revisão de 19/11 com status de cada item (21/11) |
| `notes.md`, `llm-problems-notes.md` | Notas de estudo e de alterações do usuário |
| `.vscode/` | Configuração local do editor; ignorada pelo Git, pode não existir |
| `AGENTS.md` | Instruções curtas para retomar o contexto |
| `testes/empacotar.py` | ZIP acadêmico; converte `src` → `codigo`, `ai` → `ia` |
| `testes/exportar_contexto.py` | ZIP de continuidade; preserva os nomes locais |

As pastas foram renomeadas pelo usuário durante o trabalho: `codigo/` virou
`src/`, `ia/` virou `ai/` e o diretório original virou `projeto-template/`.
Os comandos foram ajustados. Não reverta essas escolhas por confundi-las com
a estrutura de entrega do enunciado.

## Requisitos e decisões do algoritmo

- C++17. Um padrão/coluna é um recipiente válido não vazio.
- Formulação de partição: minimizar soma de `x_B`, com soma das colunas que
  cobrem cada item igual a 1. O mestre usa variáveis contínuas na relaxação.
- Gurobi é usado somente para os PLs do mestre; não executa a árvore inteira.
- A heurística First Fit probabilística do código-base foi mantida para
  incumbente e colunas iniciais; unitários garantem uma saída até com T=0.
- A geração tenta pricing guloso primeiro (densidade, valor e ordens aleatórias).
  Somente quando não adiciona colunas, chama pricing exato por enumeração
  incluir/excluir, com poda pela mochila fracionária sem conflitos.
- Duais não positivos podem ser descartados por grupo no pricing.
- Decisões Ryan–Foster são representadas por pares juntos/separados. Union-find
  contrai os juntos. Capacidade, conflitos e restrições de separados são
  propagados aos grupos. A raiz filtra o banco global; cada filho herda as
  colunas do mestre final do pai, filtradas só pelo par novo
  (`Node::columns`, `inheritColumns`). O mestre usa primal simplex (`Method=0`).
- Banco global e conjunto local evitam duplicar padrões.
- Usa cotas de Farley com limite superior do pricing. O objetivo do mestre
  restrito sozinho não é usado como LB do nó.
- Prioriza o menor limitante; empates favorecem o nó mais profundo (`Node::depth`)
  e depois a ordem de criação. Filhos herdam o limite do pai. O nó atual fica
  representado na fronteira até ser resolvido ou substituído por ambos filhos.
  Assim, uma interrupção não perde uma subárvore na cota global.
- Heurística MIP de arredondamento (`roundingHeuristics`, Seção 10.6 do
  enunciado): no nó raiz e a cada 50 nós, depois da convergência, um MIP de
  cobertura sobre o banco global de colunas, com até 5 s e Cutoff de
  incumbente−0,5. Só fornece incumbente; nunca entra no limitante inferior.
- A incumbente é verificada antes de aceitar uma solução inteira; `main`
  verifica novamente antes de gravar a saída.

Funções: `SolveBinPacking` em `binpacksolver.cpp`; `Node`, `Groups`, `Pricing`,
`certifiedBound` e `BranchAndPrice` em `branchprice.cpp`; dados e verificador
em `binpacking.h`; parsing e validação da CLI em `commandline.h/.cpp`;
leitura/gravação e coordenação em `main.cpp`.

Atualização de 19/11/2026: o parsing antes embutido em `main` foi extraído
para `ParseCommandLine`, com retorno `std::optional<CommandLineOptions>`.
Opções, validações, mensagens e código de saída foram preservados; o Makefile,
README e mapa de estudo foram atualizados. `make format-check` passou e
`make test GUROBI_HOME=/opt/gurobi1303/linux64` passou nas 187 instâncias,
incluindo os checks da CLI. Os testes exigiram execução fora do sandbox para
a licença reconhecer o HostID. `make lint` com esse Gurobi terminou sem erros,
com cinco avisos nos arquivos do solver e nenhum no módulo extraído.

Atualização de 21/11/2026: depois de 19/11 entraram o desempate por
profundidade (`bff883e`), a heurística de arredondamento (`a9ce601`), novas
instâncias `medium-1..3` e `hard-1..3` (60 a 360 itens; acima do limite de 16
itens do oráculo, por isso ficam fora da suíte), logs e o registro das
conversas com Claude Code em `ai/`. O Makefile passou a detectar Linux/macOS
e o Gurobi instalado. Verificações de 21/11 neste Linux:
`make format-check` passou; `make test` deu `OK: 187 optima certified`
(1 ramificação, 104 chamadas de pricing exato); `make lint` mostrou 6 avisos
e nenhum erro; hard-1, hard-2, hard-3 e medium-3 com `-t 3` terminaram em
3,00–3,01 s de tempo real, com saídas viáveis.

Atualização de 21/11/2026 (item 4 do relatório, parte 1): primal simplex no
mestre e herança das colunas do pai em vez de varrer o banco a cada nó.
`make test` e a rodada `--random 500` passaram (187 e 567 ótimos), mas nelas
só há uma ramificação. Um estresse à parte, com 60 instâncias geradas para
ramificar (variações de Grötzsch e grafos densos, até 16 itens), teve 119
ramificações e todas as saídas com UB=LB=OPT do oráculo `--exato`.
medium-3 com `-t 30` provou o ótimo em 2,6 s e 1,4 s (sementes 1 e 2),
contra 4,4 s e 2,2 s antes. O modelo ainda é recriado em cada nó.

Atualização de 21/11/2026 (item 2 do relatório): falha local não encerra
mais o B&P. Nós com falha vão para `parked` com a cota certificada e entram
no LB global; o status do PL é classificado (prazo × numérico, com nova
tentativa); coluna repetida e falta de par fracionário são tratadas sem
abortar. `BNP_FAULT_NODE=k` injeta falha para teste; `verificar.py` cobre isso.

Atualização de 21/11/2026 (item 3 do relatório): a FFD separa o orçamento
macio de 5% (só entre rodadas, após a primeira) do prazo T (único que
interrompe uma rodada). `verificar.py` testa n=3000, k=40000, T=0,1 s.

## Pedidos atendidos ao longo do chat

1. Implementação e organização da entrega, preservando os originais.
2. `make run` com parâmetros preenchidos e variáveis substituíveis.
3. Documentação para estudo e verificador independente de saída.
4. Correção do IntelliSense: header `gurobi_c++.h` via includePath local.
5. Logs Gurobi ativados: `OutputFlag=1`, após o usuário notar sua ausência.
   Os logs são de PL; a árvore própria aparece em `[B&P]`. Se a heurística
   já provar o ótimo ou T=0, Gurobi não é chamado.
6. Integração clang-tidy/clang-format e formatação dos fontes ativos.
   VS Code analisa automaticamente e formata ao salvar C++.
7. Exportação de contexto, instruções de continuidade e pacote portátil.
8. Revisão de bugs (`RELATORIO_BUGS.md`), desempate por profundidade,
   heurística de arredondamento, Makefile portátil e novas instâncias.

## Comandos disponíveis na raiz

```bash
make
make run
make run INPUT=../testes/instancias/ryan_foster.txt OUTPUT=../saida-rf.txt TIME=30 SEED=42
make test
make lint
make format
make format-check
python3 testes/conferir_solucao.py -f testes/instancias/exemplo.txt -o saida.txt --exato
python3 testes/verificar.py --random 500 --report /tmp/bnp-validacao.json
python3 testes/empacotar.py
python3 testes/exportar_contexto.py
```

`INPUT` e `OUTPUT` são interpretados a partir de `src/`, pois o Makefile da
raiz encaminha os alvos. Padrões: `TEST_FILE=medium-1`, entrada
`../testes/instancias/$(TEST_FILE).txt`, saída `../tests-results/$(TEST_FILE).txt`,
60 segundos, semente 1. `make empacotar` gera o ZIP acadêmico. No ZIP acadêmico extraído, use `make -C codigo ...`.

## Evidências já obtidas

- Exemplo do enunciado: UB=LB=4; confirmado também por argumento manual e DP.
- Caso `ryan_foster.txt`: junção de dois ciclos C5, LP=5 e OPT=6. A execução
  validada processou três nós e uma ramificação, exercitando os dois filhos.
- Suíte padrão: 187 instâncias comparadas com ótimo por DP independente;
  todas com UB=LB=OPT, uma ramificação e 104 chamadas de pricing exato
  (21/11/2026; eram 105 em 11/11).
- Rodada ampliada: 567 instâncias, todas com valores corretos; 255 chamadas
  de pricing exato. Relatório em `testes/resultados-ampliados.json`.
- Suíte original de 187 executada também com ASan/UBSan, sem erros reportados;
  isso foi em 11/11, antes do desempate por profundidade e da heurística de
  arredondamento. Não afirmar uma nova execução
  instrumentada para mudanças posteriores sem realizá-la.
- Após a formatação mais recente, `make format-check` e `make test` passaram.
- Último `make lint` (21/11/2026): seis avisos no código, sem erros de análise. Não foram
  aplicados `--fix` automáticos. O retorno normal de lint não falha por aviso;
  para exigir isso: `make lint TIDYFLAGS='--warnings-as-errors=*'`.
- Verificador externo rejeitou um item duplicado e LB adulterado maior que OPT.
- O pacote acadêmico foi extraído e os comandos de compilação/execução e
  conferência testados; as regras de formatação também foram verificadas nele.

Esses registros descrevem execuções realizadas; não garantem que arquivos
editados depois continuem corretos. Consulte a árvore atual e repita os checks
apropriados quando retomar. `git status` tinha alterações staged e unstaged;
nenhum commit, descarte, envio de e-mail ou publicação foi feito pelo assistente.

## Pendências e limites conhecidos

- Seis avisos de clang-tidy para revisão: `rng` estático com semente constante
  (dois avisos), exceção `TimeExpired` sem base `std::exception`, reserva de
  vetor, catch vazio e campo não inicializado em `binpacking.h:46`.
  Alguns comportamentos são intencionais, mas precisam ser examinados.
- Itens abertos de `RELATORIO_BUGS.md`: 4 (resolvido em parte: falta
  reaproveitar o modelo entre nós), 7, 8 e 9. Os itens 1, 2, 3, 5 e 6
  foram resolvidos; o 10, em parte.
- Prazo T: inicialização de licença e operações nativas são cooperativas.
  Nas medições de 21/11 o término ficou em T+0,01 s, mas não foi demonstrado
  um término rígido em T para todas as entradas, apesar do requisito literal
  do PDF.
- Instâncias grandes podem terminar com lacuna (por exemplo hard-3 com `-t 5`:
  LB=110, UB=118). A solução é viável, mas a otimalidade não foi certificada.
- Certificação numérica com tolerâncias, não aritmética racional exata.
- Cobertura limitada de árvores profundas e sem teste isolado de cada pricing.
- Grafo de grupos usa memória quadrática; pricing e árvore têm pior caso
  exponencial. Sem estabilização dual nem remoção de colunas antigas.
- Entrega individual exige domínio do aluno e conversas completas de IA.
  Cada nova sessão (Codex ou Claude Code) precisa ser exportada para `ai/`,
  registrada em `ai/README`, e o ZIP precisa ser gerado de novo.
  O pacote foi preparado localmente; não foi enviado ao professor.

## Configurar no próximo computador

Ambiente atual (21/11/2026): Linux x86_64 (CachyOS), GCC 16.2.1,
Gurobi 13.0.3 em `/opt/gurobi1303/linux64` (biblioteca `gurobi130`),
Python 3.14.10 e LLVM 23.1.5 (`/usr/bin/clang-format`, `/usr/bin/clang-tidy`).
O ambiente original (11/11) era macOS Apple Silicon, Apple clang 17.0.0,
Gurobi 13.0.2, Python 3.14.8 e LLVM 23.1.3. São versões registradas,
não uma obrigação de usar exatamente as mesmas, exceto se desejar reprodução
estrita da formatação e diagnósticos.

1. Extraia `contexto-codex.zip` em uma pasta e abra a raiz no Codex/VS Code.
2. Instale/configure compilador C++17, Make, Python 3 e Gurobi/licença local.
3. `src/Makefile` detecta Linux/macOS e usa o Gurobi mais recente em
   `/opt/gurobi*/linux64` ou `/Library/gurobi*/macos_universal2`. Para outro
   local, passe `GUROBI_HOME=...` (e, se preciso, `GUROBI_LIB=...`) ao make.
4. Se usar VS Code, ajuste `.vscode/c_cpp_properties.json` (compilador,
   arquitetura e includePath do Gurobi). A pasta `.vscode/` é ignorada pelo
   Git; a versão do Mac usava `/usr/bin/clang++` e `macos-clang-arm64`.
5. Ajuste os executáveis do clang em `.vscode/settings.json`; no Mac estavam
   em `/opt/homebrew/opt/llvm/bin/`, no Linux atual em `/usr/bin/`.
6. Rode `make`, `make test` e `make format-check` para validar o novo ambiente.
7. Inicie uma conversa com o prompt abaixo e continue a tarefa desejada.

## Prompt para o próximo Codex

> Este é o projeto de Bin Packing com Classes de Conflito que já desenvolvemos.
> Leia AGENTS.md e docs/CONTEXTO_CODEX.md, confira README e o estado atual dos
> arquivos. Consulte ai/conversa-AAAA-MM-DD.md se precisar do histórico detalhado. Não
> reimplemente do zero nem descarte alterações locais. Ajude-me a continuar
> a partir desse estado e primeiro verifique os caminhos das dependências
> neste computador. Minha próxima tarefa é: [descreva aqui].

O papel de `AGENTS.md` como instrução de projeto é documentado pela OpenAI:
https://learn.chatgpt.com/docs/agent-configuration/agents-md
O resumo e a transcrição complementam esse mecanismo. Sessões retomáveis via
`codex resume` são outro recurso; não se deve pressupor que este ZIP importe
uma sessão no histórico da interface.
