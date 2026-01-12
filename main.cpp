#include "openfhe.h"

using namespace lbcrypto;
using namespace std;

int main() {
    // 1. SETUP PARAMETERS
    // We use CKKS because it's fast for parallel "vector" math
    uint32_t multDepth = 1;
    uint32_t scaleModSize = 50;
    uint32_t batchSize = 128; // Our sorting size

    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(multDepth);
    parameters.SetScalingModSize(scaleModSize);
    parameters.SetBatchSize(batchSize);

    CryptoContext<DCRTPoly> cc = GenCryptoContext(parameters);

    // 2. ENABLE FEATURES
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE); // Required for complex rotations

    // 3. KEY GENERATION
    KeyPair<DCRTPoly> keyPair = cc->KeyGen();
    cc->EvalMultKeyGen(keyPair.secretKey);
    
    // Generate rotation keys (This allows us to move numbers inside the ciphertext)
    // For Even-Odd Merge Sort, we need power-of-two rotations
    vector<int32_t> rotationIndices = {1, 2, 4, 8, 16, 32, 64};
    cc->EvalAtIndexKeyGen(keyPair.secretKey, rotationIndices);

    // 4. ENCODE & ENCRYPT
    vector<double> input = { /* Put your 128 numbers here */ };
    // Example: Random data
    for(int i=0; i<128; i++) input.push_back(std::rand() % 100);

    Plaintext pt = cc->MakeCKKSPackedPlaintext(input);
    auto ct = cc->Encrypt(keyPair.publicKey, pt);

    cout << "Initial 128 ciphertexts prepared. Starting Sort Network..." << endl;

    // 5. THE SORTING NETWORK (Logic Flow)
    // This is where you implement the Batcher's Merge Sort logic
    // You will use cc->EvalRotate(ct, index) and Min/Max gates.
    
    /* Note: A true 128-input Even-Odd Merge Sort is a series of 
       Log2(N) stages. In each stage, we rotate the ciphertext,
       compare it with itself, and 'Select' the Min or Max.
    */

    // 6. DECRYPT & VERIFY
    Plaintext result;
    cc->Decrypt(keyPair.secretKey, ct, &result);
    result->SetLength(batchSize);

    cout << "Sorted Result: " << result << endl;

    return 0;
}