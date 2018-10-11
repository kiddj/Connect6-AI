# Keras2cpp
Keras로 학습한 모델을 C++로 불러와 inference에 사용하는 모듈

### Usage
- Keras model을 학습하고 .h5 파일로 저장 (model.save()에서 `include_optimizer=False` 여야 함. )
- convert_model.py를 실행해 모델 변환 (EX: `python convert_model.py [h5_model_name] [json_output_name]`)
- 만들어진 json파일을 cpp에서 `fdeep::load_model`로 불러옴
- 의존성을 모두 Include Path에 추가한 뒤 컴파일
`g++ -o test test.cpp -std=c++14 -I./include`
- 생성된 실행파일로 inference
