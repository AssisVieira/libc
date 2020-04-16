
# Objetivo

  Criar um ambiente de execução de atores considerando apenas uma máquina.


# Teatro

 - Responsável por orquestrar todo o ambiente de execução de atores em uma única
   máquina.

 - Executado na linha de execução principal do processo, ou CPU Principal, com 
   veremos a seguir.


# Componente: CPU

  - Considerando otimizar o uso do cache dos processadores e dos seus núcleos,
    vamos criar um componente lógico denominado por nós de CPU. O componente 
    será similar ao conceito de linha de execução (thread), porém com as 
    restrições descritas a seguir.

  - Uma CPU é um componente lógico com relação de um-para-um com um componente
    físico, de mesmo nome, capaz de tratar uma linha de execução (thread) por
    vez. Para diferenciar estes dois componentes, vamos chamar apenas de "CPU" 
    ou "CPU lógica" para nos referir a este componente e de "CPU física" para
    nos referir ao componente físico.

  - CPU física não deve ser confundido com soquetes (sockets) ou núcleos
    (cores). Uma máquina pode ter um ou mais soquetes. Um soquete pode ter um
    mais ou mais núcleos. Um núcleo pode ter uma ou mais CPUs físicas.

  - Em síntese, considere uma CPU como uma linha de execução (thread) que é 
    tratada exclusivamente por uma única CPU física. Para tal, na primeira 
    oportunidade da primeira execução da CPU, a CPU deve sinalizar ao sistema
    operacional que ela deseja ser executada por apenas uma única CPU física 
    durante toda a sua vida, de modo a evitar maiores mudanças de contexto e 
    maior tirando proveito do cache interno das CPUs.

  - A quantidade de CPUs disponíveis pode ser configurado no momento da 
    criação do processo.

  - A quantidade de CPUs não pode ser menor que 2. Pois uma CPU é usada
    exclusivamente pelo Teatro e as demais devem ser usadas para execução dos 
    atores.

  - A quantidade de CPUs não pode ser maior que a quantidade de CPUs físicas. 

  - Com exceção da CPU principal (linha de execução principal), todas as demais
    CPUs são criadas pelo Teatro logo no seu início.

# Componente: Diretor

  - Diretores são responsáveis por executar e coordenar o trabalho dos atores 
    entre as CPUs disponíveis, com exceção da CPU Principal.

  - Os diretores devem ser criados pelo Teatro, logo após a criação das CPUs.

  - Cada CPU, em todo o seu ciclo de vida, deverá executar um único Diretor, 
    com exceção da CPU Principal, pois executará o Teatro.

  - Cada diretor deverá ter uma fila de atores. 

  - Diretores ociosos poderam consumir a fila de outros diretores, se os seus
    parâmetros assim o permitir. Caso não permita, o diretor ocioso deverá 
    colocar a sua CPU para dormir, esperando o próximo ator a entrar na sua 
    fila.

  - O diretor deve executar no máximo N estados consecutivos de um mesmo ator,
    sendo N um parâmetro fornecido no momento da criação do diretor.
    Após isso:

    a) Se o ator estiver sido encerrado, o ator deve ser retirado da fila do 
       diretor e destruído.

    b) Se o ator estiver ocupado, o ator deve ser colocado no final da fila do 
       diretor.

    c) Se o ator estiver ocioso, o ator deve ser removido da fila do diretor.

  - O agendamento de atores na fila de execução do worker si dá no momento
    em que alguém envia uma mensagem para o ator. Após isso há dois
comportamentos:

  a) Se o ator estiver ocupado, ignora o agendamento.

  b) Se o ator estiver ocioso, então agenda o ator com o mesmo worker.



