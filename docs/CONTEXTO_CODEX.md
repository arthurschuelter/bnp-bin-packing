# Contexto para continuar com Codex em outro computador

Registro preparado em 11/11/2026. Leia este arquivo primeiro, depois o README
e os trechos relevantes do guia de estudo. Consulte a transcrição para
recuperar detalhes que não cabem neste resumo.

## Objetivo do usuário

Desenvolver o Projeto 2 de Programação Linear Inteira descrito em
`enunciado.pdf`, usando o código e as instâncias fornecidos como base.
O problema é Bin Packing com Classes de Conflito. O usuário também quer
entender, verificar, estudar e padronizar o trabalho.

O projeto já está implementado. O pedido mais recente é exportar o contexto
para continuar a colaboração com Codex em outro computador. Não existe uma
nova tarefa de implementar o algoritmo desde o início.

## O que este registro transfere

- Os arquivos atuais do projeto, inclusive alterações ainda não commitadas.
- As decisões técnicas, verificações e pendências descritas aqui.
- Os prompts, respostas e ferramentas disponíveis em `ai/conversa.md` e
  `ai/conversa.jsonl`, em ordem cronológica.
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
| `.vscode/` | Configuração local do IntelliSense e ferramentas |
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
  propagados aos grupos. Colunas globais são filtradas antes de reutilização.
- Banco global e conjunto local evitam duplicar padrões.
- Usa cotas de Farley com limite superior do pricing. O objetivo do mestre
  restrito sozinho não é usado como LB do nó.
- Prioriza o menor limitante; filhos herdam o limite do pai. O nó atual fica
  representado na fronteira até ser resolvido ou substituído por ambos filhos.
  Assim, uma interrupção não perde uma subárvore na cota global.
- A incumbente é verificada antes de aceitar uma solução inteira; `main`
  verifica novamente antes de gravar a saída.

Funções: `SolveBinPacking` em `binpacksolver.cpp`; `Node`, `Groups`, `Pricing`,
`certifiedBound` e `BranchAndPrice` em `branchprice.cpp`; dados e verificador
em `binpacking.h`; CLI/leitura/gravação em `main.cpp`.

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
raiz encaminha os alvos. Padrões: exemplo, saída `../saida.txt`, 60 segundos,
semente 1. No ZIP acadêmico extraído, use `make -C codigo ...`.

## Evidências já obtidas

- Exemplo do enunciado: UB=LB=4; confirmado também por argumento manual e DP.
- Caso `ryan_foster.txt`: junção de dois ciclos C5, LP=5 e OPT=6. A execução
  validada processou três nós e uma ramificação, exercitando os dois filhos.
- Suíte padrão: 187 instâncias comparadas com ótimo por DP independente;
  todas com UB=LB=OPT, uma ramificação e 105 chamadas de pricing exato.
- Rodada ampliada: 567 instâncias, todas com valores corretos; 255 chamadas
  de pricing exato. Relatório em `testes/resultados-ampliados.json`.
- Suíte original de 187 executada também com ASan/UBSan, sem erros reportados;
  isso foi antes da última formatação. Não afirmar uma nova execução
  instrumentada para mudanças posteriores sem realizá-la.
- Após a formatação mais recente, `make format-check` e `make test` passaram.
- Último `make lint`: dez avisos no código, sem erros de análise. Não foram
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

- Dez avisos de clang-tidy para revisão: gerador estático/semente constante,
  exceção `TimeExpired` sem base `std::exception`, conversões para long double,
  reserva de vetor, catch vazio e possível escape de exceção em main.
  Alguns comportamentos são intencionais, mas precisam ser examinados.
- Prazo T: inicialização de licença e operações nativas são cooperativas.
  Os testes aceitam folga temporal; não foi demonstrado um término rígido
  em T para todas as entradas, apesar do requisito literal do PDF.
- Certificação numérica com tolerâncias, não aritmética racional exata.
- Cobertura limitada de árvores profundas e sem teste isolado de cada pricing.
- Grafo de grupos usa memória quadrática; pricing e árvore têm pior caso
  exponencial. Sem estabilização dual, coleta de colunas ou MIP de arredondamento.
- Entrega individual exige domínio do aluno e conversas completas de IA.
  O pacote foi preparado localmente; não foi enviado ao professor.

## Configurar no próximo computador

O ambiente anterior era macOS Apple Silicon, Apple clang 17.0.0, Gurobi 13.0.2,
Python 3.14.8 e LLVM 23.1.3 para lint/formatação. São versões registradas,
não uma obrigação de usar exatamente as mesmas, exceto se desejar reprodução
estrita da formatação e diagnósticos.

1. Extraia `contexto-codex.zip` em uma pasta e abra a raiz no Codex/VS Code.
2. Instale/configure compilador C++17, Make, Python 3 e Gurobi/licença local.
3. Ajuste `GUROBI_HOME` e `GUROBI_LIB` em `src/Makefile` ou na linha de comando.
   O caminho antigo era `/Library/gurobi1302/macos_universal2` e `gurobi130`.
4. Ajuste `.vscode/c_cpp_properties.json`: compilador, arquitetura e includePath.
   A configuração atual usa `/usr/bin/clang++` e `macos-clang-arm64`.
5. Ajuste os executáveis em `.vscode/settings.json`; os antigos estavam em
   `/opt/homebrew/opt/llvm/bin/`. Não use esses caminhos num Linux/Windows.
6. Rode `make`, `make test` e `make format-check` para validar o novo ambiente.
7. Inicie uma conversa com o prompt abaixo e continue a tarefa desejada.

## Prompt para o próximo Codex

> Este é o projeto de Bin Packing com Classes de Conflito que já desenvolvemos.
> Leia AGENTS.md e docs/CONTEXTO_CODEX.md, confira README e o estado atual dos
> arquivos. Consulte ai/conversa.md se precisar do histórico detalhado. Não
> reimplemente do zero nem descarte alterações locais. Ajude-me a continuar
> a partir desse estado e primeiro verifique os caminhos das dependências
> neste computador. Minha próxima tarefa é: [descreva aqui].

O papel de `AGENTS.md` como instrução de projeto é documentado pela OpenAI:
https://learn.chatgpt.com/docs/agent-configuration/agents-md
O resumo e a transcrição complementam esse mecanismo. Sessões retomáveis via
`codex resume` são outro recurso; não se deve pressupor que este ZIP importe
uma sessão no histórico da interface.
