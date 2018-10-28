# Connect6-AI
인공지능프로젝트 육목 AI 설계 프로젝트

### Connectk-pure
- Connect-k의 알고리즘을 기반으로하여 만든 AI의 소스코드가 있다.
- black과 white 각각에 대한 exe를 만들 때 포트 번호를 바꿔주는 것을 잊으면 안된다.
- 컴파일 커맨드는 다음과 같다. VScode를 사용한다면 `Ctrl + Shift + B`로 바로 컴파일 할 수 있다.
```
g++ monte.cpp player.cpp utils.cpp -std=c++11 -static -lw2_32
```

### MCTS-hand-play
- MCTS와 NN을 섞은 AI이다.
- 컴파일 후에 `a.exe 2 1` 또는 `a.exe 1 2` 커맨드로 AI의 흑백을 결정하고 커맨드를 통해 겨룰 수 있다.

### MCTS-with-NN
- MCTS와 NN을 섞은 AI이다.
- 육목중계프로그램과 호환된다.
- 컴파일 하기 전에 포트 번호와 모델 파일명을 올바르게 설정했는지 재확인 해야한다.
- 컴파일 커맨드는 다음과 같다. VScode를 사용한다면 `Ctrl + Shift + B`로 바로 컴파일 할 수 있다.
```
g++ tree.cpp template.cpp game.cpp -std=c++14 -I./include -static -lws2_32
```

### NN
- Keras 기반으로 neural network를 만드는 소스 코드들이 있다.
- config.py 파일을 수정해 기보 파일들의 경로 지정해준다.
- main.py 실행으로 모델 학습 및 테스트할 수 있다.
- tensorboard를 이용한 다음과 같은 커맨드로 학습 상태를 확인할 있다.
```
tensorboard -logdir=./models -port=6006
```

### SixGo
- 실전에서 활용될 육목중계프로그램 폴더이다.
- `black.exe`와 `white.exe`를 넣고 `SixGo.exe`를 실행시키면 된다.
- NN를 넣은 AI를 사용할 경우에는 해당하는 모델 파일이 존재하는지 다시 한 번 확인해야 한다.
