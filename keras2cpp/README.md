# Keras2cpp
Keras로 학습한 모델을 C++로 불러와 inference에 사용하는 모듈

### Usage
- Keras model을 학습하고 .h5 파일로 저장 (model.save()에서 `include_optimizer=False` 여야 함. )
- convert_model.py를 실행해 모델 변환
```
python convert_model.py [h5_model_name] [json_output_name]
```
- 만들어진 json파일을 cpp에서 `fdeep::load_model`로 불러옴
- 의존성을 모두 Include Path에 추가한 뒤 컴파일
```
g++ -o test test.cpp -std=c++14 -I./include
````
- Windows 환경에서 컴파일을 진행할 경우 -static 옵션을 꼭 넣어줘야 한다.
```
g++ -o test.exe test.cpp -std=c++14 -I./include -static
```
- 생성된 실행파일로 inference

## Others
- Windows 환경에서 MINGW_64 g++를 설치했을 경우, 설치창에서 꼭 x86_64와 POSIX를 선택해야 컴파일이 진행된다. 
