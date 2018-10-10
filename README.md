# Connect6-AI
인공지능 프로젝트 육목 AI

## Deep Learning Model
MCTS에 사용될 Policy Network & Value Network 학습 및 사용. 현재 NN 폴더에 소스 코드들이 저장되어 있음. 

### Usage
- 파이썬 (가상)환경 마련 후 requirements.txt 를 이용해 의존 패키지 설치
- 육목 대국 기록을 압축 해제(game\_logs 폴더에 있음)
- NN/config.py 파일을 수정해 대국 기록 파일들의 경로 지정
- main.py 실행으로 모델 학습 및 테스트
