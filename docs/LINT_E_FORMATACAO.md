# Lint e formatação do C++

O nome da ferramenta é **clang-tidy**. Ela analisa o C++ e aponta problemas
potenciais, como uso de objetos movidos, conversões numéricas e cópias
desnecessárias. Para padronizar recuos, espaços, chaves e quebras de linha,
usamos também o **clang-format**.

Referências oficiais: [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)
e [opções do clang-format](https://clang.llvm.org/docs/ClangFormatStyleOptions.html).

## Comandos na raiz do projeto

```bash
make lint          # analisa todos os .cpp de src/ e seus headers
make format        # aplica a formatação nos .cpp e .h de src/
make format-check  # confere a formatação sem modificar arquivos
make test          # verifica os resultados do algoritmo
```

O lint não precisa compilar/linkar o executável, abrir licença nem resolver
instâncias. O Makefile passa C++17 e o caminho da instalação Gurobi ao analisador.
Os headers do Gurobi são tratados como headers de sistema, para concentrar os
avisos no código do projeto. Os arquivos originais de `projeto-template/`
não são formatados nem analisados pelos alvos acima.

O clang-tidy recebe os argumentos de compilação depois de `--`; não depende
de `compile_commands.json` nesta configuração simples. As opções de ligação,
como `-lgurobi130`, não são necessárias para essa análise.

Para conferir manualmente um arquivo:

```bash
clang-tidy src/branchprice.cpp -- -std=c++17 -isystem /Library/gurobi1302/macos_universal2/include
```

## Regras escolhidas

`.clang-tidy` habilita as famílias `clang-analyzer-*`, `bugprone-*`,
`performance-*` e a regra `modernize-use-nullptr`. Foram excluídas a regra de
parâmetros facilmente trocáveis, que é bastante abrangente para esta base,
e a recomendação contra `endl`, pois há logs que usam flush explícito.
As demais observações continuam aparecendo para revisão.

O comando `make lint` normalmente retorna zero se a análise conseguiu terminar,
mesmo que tenha encontrado avisos. Isso não significa ausência de problemas.
Para fazer os avisos falharem a verificação:

```bash
make lint TIDYFLAGS='--warnings-as-errors=*'
```

A primeira análise, com LLVM 23.1.3, apontou dez avisos no código do projeto.
Entre eles: inicialização estática do gerador, semente constante, exceção que
não herda de `std::exception`, conversões de inteiros para `long double`,
reserva de capacidade de vetor, captura vazia e propagação de exceções por
`main`. Versões diferentes podem produzir uma lista diferente.

Cada aviso precisa ser interpretado. A semente constante, por exemplo,
é intencional para permitir reprodução e o gerador recebe a semente da CLI
antes da heurística. A captura de `TimeExpired` também é intencional: a
fronteira já preserva um limitante válido. Isso não justifica ignorar todos
os avisos da mesma categoria; justifica examinar cada ocorrência.

Não foi usado `clang-tidy --fix`. Corrigir uma observação do analisador pode
exigir uma decisão sobre o código; alterações devem ser revisadas e seguidas
de `make test`. Lint e formatação não substituem o oráculo nem a prova dos
limitantes explicada em `GUIA_DE_ESTUDO.md`.

`.clang-format` usa como base o estilo LLVM, com as preferências encontradas
no `main.cpp`: chaves Allman (em linha separada), tabulação de largura 4,
limite de 100 colunas e ordenação de includes. A formatação foi aplicada aos
fontes de `src/`. Para mudar o padrão, edite esse arquivo e execute novamente
`make format`.

## VS Code

`.vscode/settings.json` habilita a análise clang-tidy pela extensão C/C++ da
Microsoft e a formatação ao salvar arquivos C++. As regras são lidas dos
arquivos `.clang-tidy` e `.clang-format` da raiz. Os avisos aparecem no painel
**Problems**. Também é possível executar **C/C++: Run Code Analysis on All Files**
pela paleta de comandos.

A configuração local aponta para os executáveis já instalados:

```text
/opt/homebrew/opt/llvm/bin/clang-tidy
/opt/homebrew/opt/llvm/bin/clang-format
```

Se mudar a instalação, ajuste esses caminhos em `.vscode/settings.json`.
A opção de formatar ao salvar fica em `[cpp].editor.formatOnSave`.

## Instalação e portabilidade

Nesta máquina, as duas ferramentas já existem, na versão Homebrew LLVM 23.1.3.
Em outro macOS com Homebrew, instale `llvm` e confira os caminhos retornados
por `brew --prefix llvm`. Se as ferramentas não estiverem no PATH:

```bash
make lint CLANG_TIDY=/opt/homebrew/opt/llvm/bin/clang-tidy
make format CLANG_FORMAT=/opt/homebrew/opt/llvm/bin/clang-format
```

O caminho Homebrew varia, por exemplo entre Apple Silicon e Intel. Os
executáveis são configuráveis no Makefile, assim como `GUROBI_HOME`:

```bash
make lint GUROBI_HOME=/caminho/do/gurobi CLANG_TIDY=/caminho/clang-tidy
```

No pacote de entrega extraído, os fontes estão em `codigo/`. Execute
`make -C codigo lint`, `make -C codigo format` e `make -C codigo format-check`.
As configurações de lint/formatação são incluídas na raiz do pacote. As
preferências locais do VS Code não são necessárias para esses comandos.

Se todos precisarem obter exatamente a mesma formatação, usem a mesma versão
do clang-format. Os Makefiles não exigem LLVM para a compilação normal: esses
executáveis só são usados pelos alvos de análise e formatação.
