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
    vector<double> input = { /* Put your 128 numbers here */ }; // stay within 0-255 to ensure precision (8 bits)
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

    /**
    * BATCHER'S ODD-EVEN MERGESORT LOGIC (128-INPUT)
    * --------------------------------------------
    * The sorting network is composed of "Stages" and "Steps".
    * For N=128 inputs:
    * * Stages: log2(128) = 7 stages.
    * Steps: Each stage 'p' has steps from 2^(p-1) down to 1.
    * * In FHE, we don't 'swap' memory addresses. We use 'Compare-and-Swap' (CAS) 
    * math gates to update the ciphertexts in place.
    */

    // Total elements to sort (Must be power of 2)
    const int n = 128;

    // STAGE LOOP: Controls the size of the blocks being merged
    for (int p = 1; p < n; p <<= 1) {
        
        // STEP LOOP: Controls the distance between elements being compared
        for (int k = p; k >= 1; k >>= 1) {
            
            // ELEMENT LOOP: Iterates through all pairs in the network
            for (int j = k % p; j <= n - 1 - k; j += (k << 1)) {
                
                // BOUNDARY LOOP: Handles the comparisons within the block
                for (int i = 0; i < std::min(k, n - j - k); ++i) {
                    
                    // Only compare elements that belong to the same 'p' group
                    if ((int)((j + i) / (p << 1)) == (int)((j + i + k) / (p << 1))) {
                        
                        /* * TARGET OPERATION: 
                        * CompareAndSwap(data[j + i], data[j + i + k]);
                        * * FHE Implementation Note:
                        * Index (j + i) and (j + i + k) represent 'slots' in our 
                        * ciphertext. We will use EvalRotate and EvalCompare 
                        * to sort them without decrypting.
                        */
                    }
                }
            }
        }
    }

    // 6. DECRYPT & VERIFY
    Plaintext result;
    cc->Decrypt(keyPair.secretKey, ct, &result);
    result->SetLength(batchSize);

    cout << "Sorted Result: " << result << endl;

    return 0;
}