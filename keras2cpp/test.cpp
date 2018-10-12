#include <ctime>
#include <algorithm>

#include "fdeep/fdeep.hpp"
#include "fplus/fplus.hpp"

template <typename T>
std::vector<size_t> sort_index(const std::vector<T> &v) {
    // Initialize original index locations
    std::vector<size_t> idx(v.size());
    std::iota(idx.begin(), idx.end(), 0);

    // Sort indexes based on comparing values in v
    std::sort(idx.begin(), idx.end(),
        [&v] (size_t i, size_t j) {
            return v[i] > v[j];
        });
    return idx;
}

int main(){
    clock_t s, e;
    const auto model = fdeep::load_model("model_50.json");
    std::vector<float> block;
    block.reserve(19 * 19 * 5);

    // Test blocks (19 x 19 x 5)
    for (int i = 0; i < 19; i++){
        for (int j = 0; j < 19; j++){
            block.push_back(1);
            for (int k = 0; k < 4; k++){
                block.push_back(0);
            }
        }
    }

    const auto shared = fdeep::shared_float_vec(fplus::make_shared_ref<fdeep::float_vec>(block));
    fdeep::tensor3 input = fdeep::tensor3(fdeep::shape_hwc(19, 19, 5), shared);
    
    s = clock();
    fdeep::tensor3s results = model.predict({input});  // tensor3s(input) -> NN -> tensor3s(output)
    fdeep::tensor3 result = results[0];  // tensor3s -> tensor3
    std::vector<float> result_vec = *result.as_vector();  // tensor3 -> vector<float>
    std::vector<size_t> sorted_index = sort_index(result_vec); // sort with keeping track of indexes
    e = clock();

    // Print index of 5 best candidates
    for(int i = 0; i < 5; i++)
        std::cout << sorted_index.at(i) << " ";
    std::cout << std::endl;
    std::cout << "Elapsed time: " << ((double) e - s) / CLOCKS_PER_SEC << std::endl;
    return 0;
}
