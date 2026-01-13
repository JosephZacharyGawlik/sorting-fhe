#include "openfhe.h"
#include <chrono> // For precise timing

using namespace lbcrypto;
using namespace std;

int main() {
    TimeVar t; // OpenFHE timer variable
    double totalTime(0);

    // 1. SETUP PARAMETERS
    uint32_t n = 8;
    // Increased depth to avoid the "DropLastElement" error
    uint32_t multDepth = 40; 
    uint32_t scaleModSize = 50;
    uint32_t batchSize = 8; 

    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(multDepth);
    parameters.SetScalingModSize(scaleModSize);
    parameters.SetBatchSize(batchSize);

    CryptoContext<DCRTPoly> cc = GenCryptoContext(parameters);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE);

    // 2. KEY GEN BENCHMARK
    TIC(t);
    KeyPair<DCRTPoly> keyPair = cc->KeyGen();
    cc->EvalMultKeyGen(keyPair.secretKey);
    vector<int32_t> rotationIndices = {1, -1, 2, -2, 4, -4};
    cc->EvalAtIndexKeyGen(keyPair.secretKey, rotationIndices);
    double keyGenTime = TOC(t);

    // 3. ENCRYPTION
    vector<double> input;
    cout << "Unsorted Data: ";
    for(int i=0; i < (int)n; i++) {
        double val = (std::rand() % 50) + 10.0;
        input.push_back(val);
        cout << val << " ";
    }
    cout << endl;

    Plaintext pt = cc->MakeCKKSPackedPlaintext(input);
    auto ct = cc->Encrypt(keyPair.publicKey, pt);

    // 4. THE SORTING NETWORK (BENCHMARKING COMPUTATION)
    cout << "Starting Encrypted Sort... (this will take a moment)" << endl;
    TIC(t);

    for (int p = 1; p < (int)n; p <<= 1) {
        for (int k = p; k >= 1; k >>= 1) {
            vector<double> maskVector(batchSize, 0.0);
            for (int j = k % p; j <= (int)n - 1 - k; j += (k << 1)) {
                for (int i = 0; i < std::min(k, (int)n - j - k); ++i) {
                    if ((int)((j + i) / (p << 1)) == (int)((j + i + k) / (p << 1))) {
                        maskVector[j + i] = 1.0;
                    }
                }
            }
            Plaintext mask = cc->MakeCKKSPackedPlaintext(maskVector);
            auto ctRotated = cc->EvalRotate(ct, k);

            // Chebyshev approximation of the Sign function
            auto diff = cc->EvalSub(ct, ctRotated);
            auto compareMask = cc->EvalChebyshevFunction([](double x) -> double {
                return x > 0 ? 1.0 : 0.0;
            }, diff, -100.0, 100.0, 5); 

            auto maxVal = cc->EvalAdd(cc->EvalMult(compareMask, ct), cc->EvalMult(cc->EvalSub(1.0, compareMask), ctRotated));
            auto minVal = cc->EvalAdd(cc->EvalMult(compareMask, ctRotated), cc->EvalMult(cc->EvalSub(1.0, compareMask), ct));

            auto keptMin = cc->EvalMult(mask, minVal);
            auto keptMax = cc->EvalMult(mask, maxVal);
            auto pushedMax = cc->EvalRotate(keptMax, -k); 

            vector<double> invMaskVec(batchSize, 1.0);
            for(int m=0; m<batchSize; m++) {
                if(maskVector[m] == 1.0 || (m >= k && maskVector[m-k] == 1.0)) invMaskVec[m] = 0.0;
            }
            Plaintext invMask = cc->MakeCKKSPackedPlaintext(invMaskVec);
            ct = cc->EvalAdd(cc->EvalAdd(keptMin, pushedMax), cc->EvalMult(ct, invMask));
        }
    }
    double computationTime = TOC(t);

    // 5. DECRYPTION
    Plaintext result;
    cc->Decrypt(keyPair.secretKey, ct, &result);
    result->SetLength(batchSize);

    // 6. PRINT STATS
    cout << "\n--- BENCHMARK RESULTS ---" << endl;
    cout << "Key Generation Time: " << keyGenTime << " ms" << endl;
    cout << "Encrypted Sort Time: " << computationTime / 1000.0 << " seconds" << endl;
    cout << "Multiplicative Depth Used: " << multDepth << endl;
    cout << "-------------------------" << endl;

    cout << "Sorted Result: ";
    auto finalValues = result->GetRealPackedValue();
    for(size_t i=0; i < finalValues.size(); i++) {
        cout << std::round(finalValues[i]) << " ";
    }
    cout << endl;

    return 0;
}