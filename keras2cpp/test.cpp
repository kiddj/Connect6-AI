#include "fdeep/fdeep.hpp"
#include "fplus/fplus.hpp"
#include <time.h>

int main(int argc, char * argv[]){

    if (argc < 1){
        printf("Usage: ./execution_file [model_name].\n");
    }

    clock_t s, e;
    const auto model = fdeep::load_model(argv[1]);
    std::vector<float> block;
    block.reserve(19*19*5);

    for (int i=0; i<19; i++){
        for (int j=0; j<19; j++){
            block.push_back(1);
            for (int k=0; k<4; k++){
                block.push_back(0);
            }
        }
    }
    const auto shared = fdeep::shared_float_vec(fplus::make_shared_ref<fdeep::float_vec>(block));
    fdeep::tensor3 t = fdeep::tensor3(fdeep::shape_hwc(19, 19, 5), shared);
    
    s = clock();
    const auto result = model.predict({t});
    e = clock();

    std::cout << fdeep::show_tensor3s(result) << std::endl;
    std::cout << "Elapsed time: " << ((double)e-s) / CLOCKS_PER_SEC << std::endl;
    return 0;
}