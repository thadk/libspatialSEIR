#include <ModelContext.hpp>
#include <LSS_FullConditionalList.hpp>
#include <OCLProvider.hpp>
#include <CompartmentalModelMatrix.hpp>
#include <CovariateMatrix.hpp>
#include <DistanceMatrix.hpp>
#include <RandomNumberProvider.hpp>
#include <IOProvider.hpp>


#ifndef BLAS_INC
#define BLAS_INC
#include<cblas.h> 
#endif

#include<cmath>
#include<ctime>

namespace SpatialSEIR
{
    using std::cout;
    using std::endl;

    ModelContext::ModelContext()
    {
        //random = new RandomNumberProvider(static_cast<unsigned int>(std::time(0)));
        random = new RandomNumberProvider(12312415);
        S_star = new CompartmentalModelMatrix();
        E_star = new CompartmentalModelMatrix();
        I_star = new CompartmentalModelMatrix();
        R_star = new CompartmentalModelMatrix();
        S = new CompartmentalModelMatrix();
        E = new CompartmentalModelMatrix();
        I = new CompartmentalModelMatrix();
        R = new CompartmentalModelMatrix();
        A0 = new InitData();
        X = new CovariateMatrix();
        X_pRS = new CovariateMatrix();
        rawDistMat = new DistanceMatrix();
        scaledDistMat = new DistanceMatrix();
        N = new int; *N = -1;
        beta = new double; *beta = -1.0;
        betaPrs = new double; *betaPrs = -1;
        eta = new double; *eta = -1.0;
        rho = new double; *rho = 0.25;
        gamma = new double; *gamma = 1.0;
        fileProvider = new IOProvider();
        isPopulated = new int; *isPopulated = 0;
        singleLocation = new int; *singleLocation = -1;
        numIterations = new int; *numIterations = 0;
        oclProvider = new OCLProvider();
        S0_OCL = new int; *S0_OCL = 0;
        I0_OCL = new int; *I0_OCL = 0;
        S_star_OCL = new int; *S_star_OCL = 0;
        E_star_OCL = new int; *E_star_OCL = 0;
        R_star_OCL = new int; *R_star_OCL = 0;
        rho_OCL = new int; *rho_OCL =0;
        beta_OCL = new int; *beta_OCL = 0;
        beta_P_RS_OCL = new int; *beta_P_RS_OCL = 0;
        model = new std::vector<FullConditional*>;

    }

    void ModelContext::setRandomSeed(unsigned int seedValue)
    {
        std::cout << "Setting seed.\n";
        delete random;
        random = new RandomNumberProvider(seedValue);
    }

    void ModelContext::setSamplingMode(int mode)
    {
        (config -> samplingMode) = mode;
    }

    int ModelContext::getSamplingMode()
    {
        return((config -> samplingMode));
    }

    void ModelContext::populate(InitData* _A0,
                                covariateArgs* xArgs, 
                                covariateArgs* xPrsArgs,
                                compartmentArgs* S_starArgs,
                                compartmentArgs* E_starArgs,
                                compartmentArgs* I_starArgs,
                                compartmentArgs* R_starArgs,
                                distanceArgs* rawDistArgs,
                                scaledDistanceArgs* scaledDistArgs,
                                double* rho_, double* beta_, 
                                double* p_ei_, double* p_ir_, double* betaPrs_, 
                                int* N_, sliceParameters* sliceWidths,
                                priorControl* priorValues,
                                modelConfiguration config_)
    {
        // Delete stuff that needs to be resized
        delete N; delete beta; delete eta; delete gamma; delete betaPrs;

        // Allocate space for the transition probabilities
        config = new modelConfiguration();
        *config = config_;
        p_se = new double[*(S_starArgs -> inRow)*(*(S_starArgs -> inCol))];
        p_se_components = new double[*(S_starArgs -> inRow)*(*(S_starArgs -> inCol))];
        compartmentCache = new double[*(S_starArgs -> inRow)*(*(S_starArgs -> inCol))];
        p_ei = new double;
        p_ir = new double;
        p_rs = new double[*(S_starArgs -> inRow)];
        N = new int[(*(S_starArgs -> inRow))*(*(S_starArgs -> inCol))];                                          
        int nbeta = (*(xArgs -> inCol_x) + (*(xArgs -> inCol_z)));
        int neta = (*(xArgs -> inRow_z));
        int nBetaPrs = *(xPrsArgs -> inCol_x);

        beta = new double[nbeta];
        betaPrs = new double[nBetaPrs];
        eta = new double[neta];
        //Depricated
        gamma = new double[*(S_starArgs -> inRow)];
        memset(gamma, 0, *(S_starArgs -> inRow)*sizeof(double));
        // Create empty compartment for calculation.
        tmpContainer = new CompartmentalModelMatrix();
        tmpContainer -> createEmptyCompartment((S_starArgs -> inRow), (S_starArgs -> inCol));

        *singleLocation = ((*(S_starArgs -> inCol)) > 1 ? 0 : 1);

        // Initialize Stuff
        A0 -> populate(_A0 -> S0,_A0 -> E0,_A0 -> I0,_A0 -> R0,_A0 -> numLocations);
        X -> genFromDataStream(xArgs -> inData_x, 
                               xArgs -> inData_z,
                               xArgs -> inRow_x,
                               xArgs -> inCol_x,
                               xArgs -> inRow_z,
                               xArgs -> inCol_z); 

        X_pRS -> genFromDataStream(xPrsArgs -> inData_x, 
                                   xPrsArgs -> inData_z,
                                   xPrsArgs -> inRow_x,
                                   xPrsArgs -> inCol_x,
                                   xPrsArgs -> inRow_z,
                                   xPrsArgs -> inCol_z); 

        S_star -> genFromDataStream(S_starArgs -> inData,
                                    S_starArgs -> inRow,
                                    S_starArgs -> inCol);

        E_star -> genFromDataStream(E_starArgs -> inData,
                                    E_starArgs -> inRow,
                                    E_starArgs -> inCol);

        I_star -> genFromDataStream(I_starArgs -> inData,
                                    I_starArgs -> inRow,
                                    I_starArgs -> inCol);

        R_star -> genFromDataStream(R_starArgs -> inData,
                                    R_starArgs -> inRow,
                                    R_starArgs -> inCol);


        S -> createEmptyCompartment(S_starArgs -> inRow,
                                    S_starArgs -> inCol);

        E -> createEmptyCompartment(S_starArgs -> inRow,
                                    S_starArgs -> inCol);

        I -> createEmptyCompartment(S_starArgs -> inRow,
                                    S_starArgs -> inCol);

        R -> createEmptyCompartment(S_starArgs -> inRow,
                                    S_starArgs -> inCol);


        rawDistMat -> genFromDataStream(rawDistArgs -> inData,
                                        rawDistArgs -> dim);

        scaledDistMat -> genFromDataStream(rawDistArgs -> inData,
                                           rawDistArgs -> dim);

        scaledDistMat -> scaledInvFunc_CPU(*(scaledDistArgs -> phi));


        // Initialize Data
        int i;
        for (i = 0; i < nbeta; i++)
        {
            beta[i] = beta_[i];
        }
        for (i = 0; i < neta; i++)
        {
            eta[i] = 0.0;
        }

        for (i = 0; i < nBetaPrs; i++)
        {
            betaPrs[i] = betaPrs_[i];
        }
        for (i = 0; i< (*(S -> nrow))*(*(S->ncol)); i++)
        {
            N[i] = N_[i];
        } 

        *rho = *rho_;
        *p_ei = *p_ei_;
        *p_ir = *p_ir_;

        // Wire up the full conditional classes
        
        S0_fc = new FC_S0(this,
                          S,
                          E,
                          E_star,
                          I_star,
                          A0,
                          p_se,
                          p_ei,
                          *(sliceWidths -> S0Width));

        E0_fc = new FC_E0(this,
                          S,
                          E,
                          I,
                          E_star,
                          I_star,
                          R_star,
                          A0,
                          p_ir,
                          p_ei,
                          p_se,
                          *(sliceWidths -> S0Width));

        I0_fc = new FC_I0(this, 
                          S,
                          I,
                          R,
                          S_star,
                          E_star,
                          R_star,
                          A0,
                          p_ir,
                          p_rs,
                          p_se,
                          *(sliceWidths -> I0Width));

        R0_fc = new FC_R0(this,
                          R,
                          S,
                          S_star,
                          E_star,
                          R_star,
                          A0,
                          p_rs,
                          p_se, 
                          *(sliceWidths -> I0Width));

        S_star_fc = new FC_S_Star(this,
                                  S_star,
                                  S,
                                  R,
                                  E_star,
                                  R_star,
                                  A0,
                                  X,
                                  p_se,
                                  p_rs,
                                  beta,
                                  rho,
                                  (S_starArgs -> steadyStateConstraintPrecision),
                                  *(sliceWidths -> S_starWidth));

        E_star_fc = new FC_E_Star(this,
                                  E_star,
                                  E,
                                  S,
                                  I_star,
                                  X,A0,p_se,p_ei,
                                  rho,beta,
                                  (E_starArgs -> steadyStateConstraintPrecision),
                                  *(sliceWidths -> E_starWidth));

        R_star_fc = new FC_R_Star(this,
                                  R_star,
                                  R,
                                  I,
                                  S_star,
                                  E_star,
                                  I_star,
                                  S,
                                  A0,p_rs,p_ir,p_se,
                                  (R_starArgs -> steadyStateConstraintPrecision),
                                  *(sliceWidths -> R_starWidth));

        beta_fc = new FC_Beta(this,
                              E_star,
                              S,
                              A0,X,p_se,beta,rho,
                              *(sliceWidths -> betaWidth),
                              (priorValues -> betaPriorPrecision));

        rho_fc = new FC_Rho(this,
                            E_star,
                            S,
                            A0,X,p_se,beta,rho,
                            *(sliceWidths -> rhoWidth));


        betaPrs_fc = new FC_Beta_P_RS(this,S_star,R,X_pRS,A0,p_rs,betaPrs, 
                                      (priorValues->betaPrsPriorPrecision), 
                                      *(sliceWidths -> betaPrsWidth));

        p_ei_fc = new FC_P_EI(this,
                              I_star,
                              E,
                              A0,p_ei,
                              (priorValues -> P_EI_priorAlpha),
                              (priorValues -> P_EI_priorBeta));

        p_ir_fc =  new FC_P_IR(this,
                             R_star,
                             I,
                             A0,p_ir,
                             (priorValues -> P_IR_priorAlpha),
                             (priorValues -> P_IR_priorBeta));

        // Calculate Compartments
        this -> calculateS_CPU();
        this -> calculateE_CPU();
        this -> calculateI_CPU();
        this -> calculateR_CPU();
        this -> calculateP_RS_CPU();
        this -> calculateP_SE_CPU();
        this -> calculateS_CPU();
        this -> calculateE_CPU();
        this -> calculateI_CPU();
        this -> calculateR_CPU();
        this -> calculateP_RS_CPU();
        this -> calculateP_SE_CPU();

        this -> buildModel();
        *isPopulated = 1;
    }

    void ModelContext::buildModel()
    {
        // build the model here. 
        if ((config -> reinfectionMode) == 1)
        {
            model -> push_back(S0_fc);
            model -> push_back(I0_fc);
            model -> push_back(S_star_fc);
            model -> push_back(E_star_fc);
            model -> push_back(R_star_fc);
            model -> push_back(beta_fc);
            model -> push_back(betaPrs_fc);
            if (!(*singleLocation))
            {
                model -> push_back(rho_fc);
            }
            model -> push_back(p_ei_fc);
            model -> push_back(p_ir_fc);
        }
        else if (config -> reinfectionMode == 2)
        {
            model -> push_back(S0_fc);
            model -> push_back(I0_fc);
            model -> push_back(S_star_fc);
            model -> push_back(E_star_fc);
            model -> push_back(R_star_fc);
            model -> push_back(beta_fc);
            if (!(*singleLocation))
            {
                model -> push_back(rho_fc);
            }
            model -> push_back(p_ei_fc);
            model -> push_back(p_ir_fc);
        }
        else if (config -> reinfectionMode  == 3)
        {
            model -> push_back(S0_fc);
            model -> push_back(I0_fc);
            model -> push_back(E_star_fc);
            model -> push_back(R_star_fc);
            model -> push_back(beta_fc);
            if (!(*singleLocation))
            {
                model -> push_back(rho_fc);
            }
            model -> push_back(p_ei_fc);
            model -> push_back(p_ir_fc);
        } 
    }

    int ModelContext::checkCompartmentBounds()
    {
        int i;
        int err = 0;
        int rowCol = (*(R->ncol))*(*(R->nrow));
        for (i = 0; i < rowCol;i++)
        {
            if ((S_star -> data)[i] > (R -> data)[i])
            {
                std::cout << "S_star too big: " << i << ", val:"<< S_star_fc -> getValue() << " \n";
                err = 1;
                break;
            }
            if ((S_star -> data)[i] < 0)
            {
                std::cout << "S_star <0: " << i << ", val:"<< S_star_fc -> getValue() << " \n";
                err = 1;
                break;
            }
            if ((S -> data)[i] < 0)
            {
                std::cout << "S <0: " << i << " \n";
                err = 1;
                break;
            }

        }
        for (i = 0; i < rowCol;i++)
        {
            if ((E_star -> data)[i] > (S -> data)[i])
            {
                std::cout << "E_star too big: " << i << ", val:"<< E_star_fc -> getValue() << " \n";
                err = 1;
                break;
            }
            if ((E_star -> data)[i] < 0)
            {
                std::cout << "E_star <0: " << i << ", val:"<< E_star_fc -> getValue() << " \n";
                err = 1;
                break;
            }
            if ((E -> data)[i] < 0)
            {
                std::cout << "E <0: " << i << " \n";
                err = 1;
                break;
            }

        }
        for (i = 0; i < rowCol;i++)
        {
            if ((I_star -> data)[i] > (E -> data)[i])
            {
                std::cout << "I_star too big: " << i << "\n";
                err = 1;
                break;
            }
            if ((I_star -> data)[i] < 0)
            {
                std::cout << "I_star <0: " << i << ", val: \n";
                err = 1;
                break;
            }

            if ((I -> data)[i] < 0)
            {
                std::cout << "I_star <0: " << i << " \n";
                err = 1;
                break;
            }

        }
        for (i = 0; i < rowCol;i++)
        {
            if ((R_star -> data)[i] > (I -> data)[i])
            {
                
                std::cout << "R_star too big: " << i << ", val:"<< R_star_fc -> getValue() << " \n";
                err = 1;
                break;
            }
            if ((R_star -> data)[i] < 0)
            {
                std::cout << "R_star <0: " << i << ", val:"<< R_star_fc -> getValue() << " \n";
                err = 1;
                break;
            }
            if ((R -> data)[i] < 0)
            {
                std::cout << "R <0: " << i << " \n";
                err = 1;
                break;
            }
        }
        int nPrs = *(I_star -> nrow);
        for (i = 0; i < nPrs;i++)
        {
            if (p_rs[i] <= 0 || p_rs[i] >= 1)
            {
                std::cout << "Invalid P_RS Value at Index" << i << "\n";
                err = 1;
            }
        }

        return(err);
    }

    void ModelContext::printFCValues()
    {
        beta_fc -> evalCPU();
        betaPrs_fc -> evalCPU();
        p_ei_fc -> evalCPU();
        p_ir_fc -> evalCPU();
        rho_fc -> evalCPU();
        std::cout << "  FC Values:\n";
        std::cout << "    Beta: " << beta_fc ->getValue() << "\n";
        std::cout << "    betaPrs: " << betaPrs_fc -> getValue() << "\n";
        std::cout << "    p_ei: " << p_ei_fc ->getValue() << "\n";
        std::cout << "    p_ir: " << p_ir_fc ->getValue() << "\n";
        std::cout << "    rho: " << rho_fc ->getValue() << "\n";
    }

    void ModelContext::updateSamplingParameters(double desiredRatio, double targetWidth, double proportionChange)
    {
        S0_fc -> updateSamplingParameters(desiredRatio, targetWidth, proportionChange);     
        I0_fc -> updateSamplingParameters(desiredRatio, targetWidth, proportionChange);  
        S_star_fc -> updateSamplingParameters(desiredRatio, targetWidth, proportionChange);  
        E_star_fc -> updateSamplingParameters(desiredRatio, targetWidth, proportionChange); 
        R_star_fc -> updateSamplingParameters(desiredRatio, targetWidth, proportionChange); 
        beta_fc -> updateSamplingParameters(desiredRatio, targetWidth, proportionChange); 
        betaPrs_fc -> updateSamplingParameters(desiredRatio, targetWidth, proportionChange); 
        if (!(*singleLocation))
        {
            rho_fc -> updateSamplingParameters(desiredRatio, targetWidth, proportionChange); 
        }
    }

    void ModelContext::simulationIter(bool verbose = false, bool debug = false)
    {
        if (debug)
        {
            this -> printFCValues();
            this -> checkCompartmentBounds();

            std::cout << "S: " << S -> marginSum(3,-1) << "\n";
            std::cout << "E: " << E -> marginSum(3,-1) << "\n";
            std::cout << "I: " << I -> marginSum(3,-1) << "\n";
            std::cout << "R: " << R -> marginSum(3,-1) << "\n";

            std::cout << "S_star: " << S_star -> marginSum(3,-1) << "\n";
            std::cout << "E_star: " << E_star -> marginSum(3,-1) << "\n";
            std::cout << "I_star: " << I_star -> marginSum(3,-1) << "\n";
            std::cout << "R_star: " << R_star -> marginSum(3,-1) << "\n";
        }

        if (verbose){std::cout << "Sampling S0\n";}
        if (!(*S0_OCL)){S0_fc -> sampleCPU();}
        else {S0_fc -> sampleOCL();}

        /*
        if (verbose){std::cout << "Sampling E0\n";}
        if (!(*S0_OCL)){E0_fc -> sampleCPU();}
        else {E0_fc -> sampleOCL();}
        */

        if (verbose){std::cout << "Sampling I0\n";}
        if (!(*I0_OCL)){I0_fc -> sampleCPU();}
        else {I0_fc -> sampleOCL();}

        /*
        if (verbose){std::cout << "Sampling R0\n";}
        if (!(*I0_OCL)){R0_fc -> sampleCPU();}
        else {R0_fc -> sampleOCL();}
        */ 

        if ((config -> reinfectionMode) <= 2)
        {
            if (verbose){std::cout << "Sampling S_star\n";}
            if (!(*S_star_OCL)){S_star_fc -> sampleCPU();}
            else {S_star_fc -> sampleOCL();}
        }

        if ((config -> reinfectionMode) == 1)
        {
            if (verbose){std::cout << "Sampling betaPrs\n";}
            if (!(*beta_P_RS_OCL)){betaPrs_fc -> sampleCPU();}
            else {betaPrs_fc -> sampleOCL();}
        }

        if (verbose){std::cout << "Sampling E_star\n";}
        if (!(*E_star_OCL)){E_star_fc -> sampleCPU();}
        else {E_star_fc -> sampleOCL();}

        if (verbose){std::cout << "Sampling R_star\n";}
        if (!(*R_star_OCL)){R_star_fc -> sampleCPU();}
        else {R_star_fc -> sampleOCL();}

        if (verbose){std::cout << "Sampling beta\n";}
        if (!(*beta_OCL)){beta_fc -> sampleCPU();}
        else {beta_fc -> sampleOCL();}
 
        if (verbose){std::cout << "Sampling p_ei\n";}
        p_ei_fc -> sampleCPU();


        if (verbose){std::cout << "Sampling p_ir\n";}
        p_ir_fc -> sampleCPU();

        if (!(*singleLocation))
        {
            // Spatial dependence doesn't apply to single spatial unit. 
            if (verbose){std::cout << "Sampling rho\n";}
            if (!(*rho_OCL)){rho_fc -> sampleCPU();}
            else {rho_fc -> sampleOCL();}
        }
    }

    // Method: runSimulation
    // Accesses: Everything lol
    // Updates: Everything lol
    void ModelContext::runSimulation(int nIterations, bool verbose = false, bool debug = false)
    {
        int i;
        int itrStart = *numIterations;
        int itrMax = nIterations + (*numIterations);
        for (i = itrStart; i < itrMax; i++)
        {
            if (verbose)
            {
                std::cout << "Iteration: " << i << "\n";
            }
            this -> simulationIter(verbose, debug);
            this -> fileProvider -> catIter(i);
            (*numIterations) = (*numIterations + 1);
        }
    }

    // Method: calculateS
    // Accesses: A0, S_star, E_star
    // Updates: S
    void ModelContext::calculateS_CPU()
    {
        calculateGenericCompartment_CPU(S, A0 -> S0,
                                    S_star, E_star);
    }
    void ModelContext::calculateS_CPU(int startLoc, int startTime)
    {
        calculateGenericCompartment_CPU(S, A0 -> S0,
                                    S_star, E_star,
                                    startLoc, startTime);
    }

    void ModelContext::calculateS_givenE_CPU()
    {
        int i;
        int nLoc = *(S -> ncol);
        int nTpt = *(S -> nrow);
        int maxItr = nLoc*nTpt;
        for (i = 0; i < nLoc; i++)
        {
            (A0 -> S0)[i] = N[i*nTpt] - (A0 -> E0)[i] - (A0 -> I0)[i] - (A0 -> R0)[i];
        }

        for (i = 0; i < maxItr; i++)
        {
            (S->data)[i] = N[i] - (E->data)[i] - (I->data)[i] - (R->data)[i]; 
        }
    }

    void ModelContext::calculateS_givenE_CPU(int startLoc, int startTime)
    {
        int i,startIdx,idx;
        int nTpt = *(S -> nrow);
        startIdx = startTime + startLoc*nTpt;
        idx = startIdx;
        if (startTime == 0)
        {
            (A0 -> S0)[startLoc] = (N[startLoc*nTpt] - (A0 -> E0)[startLoc] - (A0 -> I0)[startLoc]
                                    - (A0 -> R0)[startLoc]);
        }

        for (i = startTime; i < *(S->nrow); i++)
        {
            (S -> data)[idx] = N[idx] - (R->data)[idx] - (E->data)[idx] - (I->data)[idx];  
            idx += 1;
        }
    }

    void ModelContext::calculateS_OCL()
    {
        throw(-1);
    }

    // Method: calculateE
    // Accesses: A0, I_star, E_star, 
    // Updates: E

    void ModelContext::calculateE_CPU()
    {
        calculateGenericCompartment_CPU(E, A0 -> E0,
                                        E_star, I_star);
    }

    void ModelContext::calculateE_CPU(int startLoc, int startTime)
    {
        calculateGenericCompartment_CPU(E, A0 -> E0,
                                        E_star, I_star,
                                        startLoc, startTime);
    }

    void ModelContext::calculateE_givenI_CPU()
    {
        int i;
        int nLoc = *(E -> ncol);
        int nTpt = *(E -> nrow);
        int maxItr = nLoc*nTpt;
        for (i = 0; i < nLoc; i++)
        {
            (A0 -> E0)[i] = N[i*nTpt] - (A0 -> S0)[i] - (A0 -> I0)[i] - (A0 -> R0)[i];
        }

        for (i = 0; i < maxItr; i++)
        {
            (E->data)[i] = N[i] - (S->data)[i] - (I->data)[i] - (R->data)[i]; 
        }
    }


    void ModelContext::calculateE_givenI_CPU(int startLoc, int startTime)
    {
        int i,startIdx,idx;
        int nTpt = (*(E -> nrow));
        startIdx = startTime + startLoc*nTpt;
        idx = startIdx;

        if (startTime == 0)
        {
            (A0 -> E0)[startLoc] = (N[idx] - (A0 -> S0)[startLoc] - (A0 -> I0)[startLoc]
                                    - (A0 -> R0)[startLoc]);
        }
        for (i = startTime; i < *(S->nrow); i++)
        {
            (E -> data)[idx] = N[idx] - (S->data)[idx] - (R->data)[idx] - (I->data)[idx];  
            idx += 1;
        }
    }


    void ModelContext::calculateE_OCL()
    {
        throw(-1);
    }

    // Method: calculateI
    // Accesses: A0, I_star, R_star
    // Updates: I
    void ModelContext::calculateI_CPU()
    {
        calculateGenericCompartment_CPU(I, A0 -> I0,
                                        I_star, R_star); 
    }

    void ModelContext::calculateI_CPU(int startLoc, int startTime)
    { 
        calculateGenericCompartment_CPU(I, A0 -> I0,
                                        I_star, R_star,
                                        startLoc, startTime);
    }

    void ModelContext::calculateI_givenR_CPU()
    {
        int i;
        int nLoc = *(I -> ncol);
        int nTpt = *(I -> nrow);
        int maxItr = nLoc*nTpt;
        for (i = 0; i < nLoc; i++)
        {
            (A0 -> I0)[i] = N[i*nTpt] - (A0 -> S0)[i] - (A0 -> E0)[i] - (A0 -> R0)[i];
        }

        for (i = 0; i < maxItr; i++)
        {
            (I->data)[i] = N[i] - (S->data)[i] - (E->data)[i] - (R->data)[i]; 
        }
    }

    void ModelContext::calculateI_givenR_CPU(int startLoc, int startTime)
    {
        int i,startIdx,idx;
        int nTpt = (*(E -> nrow));
        startIdx = startTime + startLoc*nTpt;
        idx = startIdx;
        if (startTime == 0)
        {
            (A0 -> I0)[startLoc] = (N[idx] - (A0 -> S0)[startLoc] - (A0 -> E0)[startLoc]
                                    - (A0 -> R0)[startLoc]);
        }

        for (i = startTime; i < *(S->nrow); i++)
        {
            (I -> data)[idx] = N[idx] - (S->data)[idx] - (R->data)[idx] - (E->data)[idx];  
            idx += 1;
        }
    }


    void ModelContext::calculateI_OCL()
    {
        throw(-1);
    }

    // Method: calculateR
    // Accesses: A0, R_star, S_star
    // Updates: R
    void ModelContext::calculateR_CPU()
    {

        calculateGenericCompartment_CPU(R, A0 -> R0,
                                        R_star, S_star);

    }

    void ModelContext::calculateR_CPU(int startLoc, int startTime)
    {

        calculateGenericCompartment_CPU(R, A0 -> R0,
                                        R_star, S_star,
                                        startLoc, startTime);

    }

    void ModelContext::calculateR_givenS_CPU()
    {

        int i;
        int nTpt = *(I -> nrow);
        int nLoc = *(I -> ncol);
        int maxItr = nLoc*nTpt;
        for (i = 0; i < nLoc; i++)
        {
            (A0 -> R0)[i] = N[i*nTpt] - (A0 -> S0)[i] - (A0 -> E0)[i] - (A0 -> I0)[i];
        }

        for (i = 0; i < maxItr; i++)
        {
            (R->data)[i] = N[i] - (S->data)[i] - (E->data)[i] - (I->data)[i]; 
        }
    }

    void ModelContext::calculateR_givenS_CPU(int startLoc, int startTime)
    {
        int i,startIdx,idx;
        int nTpt = *(I -> nrow);
        startIdx = startTime + startLoc*nTpt;
        idx = startIdx;
        if (startTime == 0)
        {
            (A0 -> R0)[startLoc] = (N[startLoc*nTpt] - (A0 -> S0)[startLoc] - (A0 -> E0)[startLoc]
                                    - (A0 -> I0)[startLoc]);
        }
        for (i = startTime; i < *(S->nrow); i++)
        {
            (R -> data)[idx] = N[idx] - (S->data)[idx] - (E->data)[idx] - (I->data)[idx];  
            idx += 1;
        }
    }

    void ModelContext::calculateR_OCL()
    {
        throw(-1);
    }


    // Method: calculateGenericCompartment
    // Access: A0, compartments linked by compStar poitners
    // Updates: Compartment linked by comp pointer 
    void ModelContext::calculateGenericCompartment_CPU(CompartmentalModelMatrix *comp,int *comp0, 
                                                   CompartmentalModelMatrix *compStarAdd, 
                                                   CompartmentalModelMatrix *compStarSub)
    {
        int i,j,idx1,idxL1;
        int numLoc = *(comp -> ncol);
        int numTpts = *(comp -> nrow);

        for (i = 0; i < numLoc; i++)
        {

            (comp -> data)[i*numTpts] = ((comp0)[i]);
            idx1 = i*numTpts;
            for (j = 1; j < numTpts; j++)
            {
                idxL1 = idx1;
                idx1++;
                (comp -> data)[idx1] = (comp -> data)[idxL1] + 
                                       (compStarAdd -> data)[idxL1] - 
                                       (compStarSub -> data)[idxL1];
            }
        }
    }

    void ModelContext::calculateGenericCompartment_CPU(CompartmentalModelMatrix *comp,int *comp0, 
                                                   CompartmentalModelMatrix *compStarAdd, 
                                                   CompartmentalModelMatrix *compStarSub,
                                                   int startLoc, int startTime)
    {
        int j,idx1,idxL1;
        int startIdx;
        int numTpts = *(comp -> nrow);

        startIdx = startLoc*numTpts + startTime; 

        if (startTime == 0)
        {
            (comp -> data)[startIdx] = ((comp0)[startLoc]);
            idx1 = startLoc*numTpts;
            for (j = 1; j < numTpts; j++)
            {
                idxL1 = idx1;
                idx1++;
                (comp -> data)[idx1] = (comp -> data)[idxL1] + 
                                       (compStarAdd -> data)[idxL1] - 
                                       (compStarSub -> data)[idxL1];
            }
        }
        else
        {
            idx1 = startLoc*numTpts + startTime;
            idxL1 = idx1 - 1; 
            for (j = startTime; j<numTpts; j++)
            {

                
                (comp -> data)[idx1] = (comp -> data)[idxL1] + 
                                       (compStarAdd -> data)[idxL1] - 
                                       (compStarSub -> data)[idxL1];    
                idxL1 = idx1;
                idx1++;
            }
        }
    }
 
    void ModelContext::calculateGenericCompartment_OCL(int *comp,int *comp0, 
                                                   int *compStarAdd, int *compStarSub)
    {
        throw(-1);
    }

    // Method: calculateP_RS
    // Accesses: betaPrs, X_pRS
    // Updates: p_rs
    void ModelContext::calculateP_RS_CPU()
    {
        int i;
        int neta = *(X_pRS -> nrow_x);
        X_pRS -> calculate_fixed_eta_CPU(p_rs, betaPrs);
        for (i = 0; i < neta; i++)
        {
            p_rs[i] = exp(-p_rs[i]);
        }
    }

    // Method: calculatePi
    // Accesses: beta, I, N, distMat, rho
    // Updates: p_se
    void ModelContext::calculateP_SE_CPU()
    {
        this -> cacheP_SE_Calculation(); 
        int i, j, index;

        // Calculate dmu: I/N * exp(eta)
        int nLoc = *(S -> ncol);
        int nTpt = *(S -> nrow);


        memset(p_se, 0, nLoc*nTpt*sizeof(double));
        // Calculate rho*sqrt(idmat)

        SpatialSEIR::matMult(this -> p_se, 
                p_se_components, 
                scaledDistMat -> data, 
                *(I -> nrow),
                *(I -> ncol), 
                *(scaledDistMat -> numLocations), 
                *(scaledDistMat -> numLocations),
                false,false, 
                *(I->nrow), 
                *(scaledDistMat -> numLocations),
                *(I->nrow));

        for (i = 0; i < nLoc; i++) 
        {
            index = i*nTpt;
            for (j = 0; j < nTpt; j++)
            {
                p_se[index] = 1-exp(-gamma[j] - p_se_components[index] - (*rho)*p_se[index]);
                index++;
            }
        }        
    }

    // To be used when beta is fixed, eta has already been exponentiated,
    // only change is to I compartment. 
    void ModelContext::calculateP_SE_CPU(int startLoc, int startTime)
    {

        int i, j, index;

        // Calculate dmu: I/N * exp(eta)
        int nLoc = *(S -> ncol);
        int nTpt = *(S -> nrow);

        index = startLoc*nTpt + startTime;
        for (j = startTime; j < nTpt; j++)
        {
            p_se_components[index] = 
               ((I -> data)[index] * (eta[index]))/N[index];
            index++;
        }

 

        //memset(p_se, 0, nLoc*nTpt*sizeof(double));
        // Calculate rho*sqrt(idmat)
        SpatialSEIR::matMult(&(p_se[startTime]), 
                &(p_se_components[startTime]), 
                scaledDistMat -> data, 
                (*(I -> nrow) - startTime),
                *(I -> ncol), 
                *(scaledDistMat -> numLocations), 
                *(scaledDistMat -> numLocations),
                false,false, 
                *(I->nrow), 
                *(scaledDistMat -> numLocations),
                *(I->nrow));

        for (i = 0; i < nLoc; i++) 
        {
            index = i*nTpt + startTime;
            for (j = startTime; j < nTpt; j++)
            {
                p_se[index] = 1-exp(-gamma[j] - p_se_components[index] - (*rho)*p_se[index]);
                index++;
            }
        }        

    }

    void ModelContext::cacheP_SE_Calculation()
    {
        int i, j, index;
        //Update Eta
        this -> X -> calculate_eta_CPU(eta, beta);

        //Exponentiate
        int nrowz = *(X->nrow_z);
        for (i = 0; i < nrowz; i++)
        {
            eta[i] = std::exp(eta[i]);
        }

        // Calculate dmu: I/N * exp(eta)
        int nLoc = *(S -> ncol);
        int nTpt = *(S -> nrow);

        for (i = 0; i < nLoc; i++) 
        {
            index = i*nTpt;
            for (j = 0; j < nTpt; j++)
            {

                p_se_components[index] = 
                   ((I -> data)[index] * (eta[index]))/N[index];
                index++;
            }
        }
    }

    void ModelContext::calculateP_SE_OCL()
    {
        oclProvider -> calculateP_SE(this); 
    }

    ModelContext::~ModelContext()
    {
        delete isPopulated;
        delete singleLocation;
        delete numIterations;
        delete fileProvider;
        delete random;
        delete S0_fc;
        delete E0_fc;
        delete I0_fc;
        delete R0_fc;
        delete S_star_fc;
        delete E_star_fc;
        delete R_star_fc;
        delete beta_fc;
        delete rho_fc;
        delete betaPrs_fc;
        delete p_ei_fc;
        delete p_ir_fc;
        delete S_star;
        delete E_star;
        delete I_star;
        delete R_star;
        delete S;
        delete E;
        delete I;
        delete R;
        delete A0;
        delete X;
        delete X_pRS;
        delete betaPrs;
        delete tmpContainer;
        delete rawDistMat;
        delete scaledDistMat;
        delete[] beta;
        delete[] gamma;
        delete[] eta;
        delete[] p_se;
        delete[] p_se_components;
        delete[] compartmentCache;
        delete p_ei;
        delete p_ir;
        delete[] p_rs;
        delete rho;
        delete config;
        delete oclProvider;
        delete S0_OCL;
        delete I0_OCL;
        delete S_star_OCL;
        delete E_star_OCL;
        delete R_star_OCL;
        delete rho_OCL;
        delete beta_OCL;
        delete beta_P_RS_OCL;
        delete[] model;
    }
}


