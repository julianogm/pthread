# pthread
Trabalho sobre pthreads para a disciplina de Tópicos em Redes. 

O código inverte os numeros de um determinado arquivo de entrada (gerado pela funçao gerar_entrada) e salva isso em um arquivo de saida.
Isso é feito utilizando buffers e mutexes nas 3 funções principais (leitura, escrita e processamento). 

### h5 Na função de leitura:
            Loop interno que fica verificando se existem dados para serem lidos no arquivo.
            Trava o mutex do buffer de entrada logo no inicio dos loops, e destrava no final destes.
            Le o buffer estiver vazio, entao verifica se o arquivo ainda tem dados.
            Caso tenha, lê para o buffer, e passa a variável de controle do buffer para 1.
            Caso não tenha mais dados no arquivo de entrada, então passa a variável de controle
            de encerramento para 2, indicando que acabou a leitura, e sai do loop interno,
            ficando apenas no loop externo esperando a finalização da thread.

### h5 Na função de escrita: 
            Loop interno que fica verificando se existem dados para serem lidos no arquivo.
            Trava o mutex do buffer de entrada logo no inicio dos loops, e destrava no final destes.
            Le o buffer estiver vazio, entao verifica se o arquivo ainda tem dados.
            Caso tenha, lê para o buffer, e passa a variável de controle do buffer para 1.
            Caso não tenha mais dados no arquivo de entrada, então passa a variável de controle
            de encerramento para 2, indicando que acabou a leitura, e sai do loop interno,
            ficando apenas no loop externo esperando a finalização da thread.

### h5 Na função de processamento:
            Dois loops internos, onde fica verificando se o buffer de saída está vazio
            e outro se o buffer de entrada está cheio.
            Os mutexes dos buffers são travados antes de cada verificação e destravados após as mesmas.                                                                                                     
            No loop do buffer de entrada, caso o buffer esteja cheio, transfere os dados do buffer para
            um buffer auxiliar, zera o buffer de entrada, indica que este agora esta vazio, destrava o mutex
            e para o loop interno do buffer de entrada. Caso o buffer de entrada esteja vazio, verifica se ainda
            existem dados para serem lidos do arquivo de entrada. Caso não, então finaliza o processamento,
            e passa a variavel de controle de encerramento para 2.
            No loop do buffer de saída, caso o buffer esteja vazio, inverte os dados do buffer auxiliar
            para o buffer de saída, indica que este agora está cheio, e para o loop interno do buffer de saída.
