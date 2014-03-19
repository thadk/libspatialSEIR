#ifndef SPATIALSEIR_INCLUDEFILES
#define SPATIALSEIR_INCLUDEFILES

#include<iostream>
#include<stdio.h>
#include<cstring>
#include<vector>
#endif

#ifndef OCL_PROVIDER_INC
#define OCL_PROVIDER_INC
#include "OCLProvider.hpp"
#endif

#ifndef COMPARTMENTAL_MODEL_MATRIX_INC
#define COMPARTMENTAL_MODEL_MATRIX_INC
#include "CompartmentalModelMatrix.hpp"
#endif

#ifndef COVARIATE_MATRIX_INC
#define COVARIATE_MATRIX_INC
#include "CovariateMatrix.hpp"
#endif

#ifndef MODEL_CONTEXT_INC
#define MODEL_CONTEXT_INC
#include "ModelContext.hpp"
#endif




namespace SpatialSEIR
{
    using std::cout;
    using std::endl;

    class ModelContext;
    int matMult(double* output, double * A, double * B, int Arow, int Acol, int Brow, int Bcol, bool TransA, bool TransB );


    class InitData
    {
        public:
            InitData(int *_S0,
                        int *_E0,
                        int *_I0,
                        int *_R0,
                        int *_S_star0,
                        int *_E_star0,
                        int *_I_star0,
                        int *_R_star0,
                        int *nLoc);
            InitData();
            void populate(int *_S0,
                          int *_E0,
                          int *_I0,
                          int *_R0,
                          int *_S_star0,
                          int *_E_star0,
                          int *_I_star0,
                          int *_R_star0,
                          int *nLoc);

            ~InitData();
            int *S0;
            int *E0;
            int *I0;
            int *R0;
            int *S_star0;
            int *E_star0;
            int *I_star0;
            int *R_star0;
            int *numLocations;
    };

    class FullConditional
    {
        public:
            //Methods
            FullConditional();            
            ~FullConditional(); 
            int evalCPU();
            int evalOCL();
            int sampleCPU();
            int sampleOCL();
            //Attributes
            double *value;
    };

    class FC_S_Star : public FullConditional 
    {
        public:
            FC_S_Star(ModelContext * _context,
                      CompartmentalModelMatrix *_S_star, 
                      CompartmentalModelMatrix *_E_star, 
                      CompartmentalModelMatrix *_R_star,
                      InitData *_A0,
                      CovariateMatrix *_X,
                      double *_p_se,
                      double *_p_rs,
                      double *_beta,
                      double *_rho);
            virtual int evalCPU();
            virtual int evalOCL();
            virtual int sampleCPU();
            virtual int sampleOCL();
            ~FC_S_Star();

            ModelContext **context;
            CompartmentalModelMatrix **S_star; 
            CompartmentalModelMatrix **E_star; 
            CompartmentalModelMatrix **R_star;
            InitData **A0;
            CovariateMatrix **X;
            double **p_se;
            double **p_rs;
            double **beta; 
            double **rho;
            double* value;

    };

    class FC_E_Star : public FullConditional 
    {
        public:
            FC_E_Star(ModelContext *_context,
                      CompartmentalModelMatrix *_S_star, 
                      CompartmentalModelMatrix *_E_star, 
                      CompartmentalModelMatrix *_I_star, 
                      CovariateMatrix *_X,
                      InitData *_A0,
                      double *_p_se,
                      double *_p_rs,
                      double *_rho,
                      double *_beta);
            ~FC_E_Star();

            virtual int evalCPU();
            virtual int evalOCL();
            virtual int sampleCPU();
            virtual int sampleOCL();

            ModelContext **context;
            CompartmentalModelMatrix **S_star; 
            CompartmentalModelMatrix **E_star; 
            CompartmentalModelMatrix **I_star; 
            CovariateMatrix **X;
            InitData **A0;
            double **p_se;
            double **p_rs;
            double **rho;
            double **beta;
            double* value;

    };

    class FC_R_Star : public FullConditional
    {
        public:
            FC_R_Star(ModelContext *_context,
                      CompartmentalModelMatrix *_R_star,
                      CompartmentalModelMatrix *_S_star,
                      CompartmentalModelMatrix *_I_star,
                      InitData *_A0,
                      double *_p_rs,
                      double *_p_ir);
            ~FC_R_Star();

            virtual int evalCPU();
            virtual int evalOCL();
            virtual int sampleCPU();
            virtual int sampleOCL();

            ModelContext **context;
            CompartmentalModelMatrix **R_star;
            CompartmentalModelMatrix **S_star;
            CompartmentalModelMatrix **I_star;
            InitData **A0;
            double **p_rs;
            double **p_ir;
            double* value;

    };

    class FC_Beta : public FullConditional 
    {
        public:
            FC_Beta(ModelContext *_context,
                    CompartmentalModelMatrix *_E_star, 
                    CompartmentalModelMatrix *_S_star, 
                    InitData *_A0,
                    CovariateMatrix *_X,
                    double *_p_se, 
                    double *_beta,
                    double *_rho); 
            ~FC_Beta();

            virtual int evalCPU();
            virtual int evalOCL();
            virtual int sampleCPU();
            virtual int sampleOCL();

            ModelContext **context;
            CompartmentalModelMatrix **E_star; 
            CompartmentalModelMatrix **S_star; 
            InitData **A0;
            CovariateMatrix **X;
            double **p_se;
            double **beta;
            double **rho;
            double* value;

    };

    class FC_P_RS : public FullConditional
    {
        public:
            FC_P_RS(ModelContext *_context,
                    CompartmentalModelMatrix *_S_star,
                    CompartmentalModelMatrix *_R_star,
                    InitData *_A0,
                    double *_p_rs 
                    );
            ~FC_P_RS();
            virtual int evalCPU();
            virtual int evalOCL();
            virtual int sampleCPU();
            virtual int sampleOCL();
            ModelContext **context;
            CompartmentalModelMatrix **S_star;
            CompartmentalModelMatrix **R_star;
            InitData **A0;
            double **p_rs;
            double* value;

    };

    class FC_Rho : public FullConditional
    {
        public:
            FC_Rho(ModelContext *_context,
                   CompartmentalModelMatrix *_S_star, 
                   CompartmentalModelMatrix *_E_star, 
                   InitData *_A0,
                   CovariateMatrix *_X,
                   double *_p_se, 
                   double *_beta, 
                   double *_rho
                   );
            ~FC_Rho();
            virtual int evalCPU();
            virtual int evalOCL();
            virtual int sampleCPU();
            virtual int sampleOCL();
            ModelContext **context;
            CompartmentalModelMatrix **S_star; 
            CompartmentalModelMatrix **E_star; 
            InitData **A0;
            CovariateMatrix **X;
            double **p_se;
            double **beta;
            double **rho;
            double* value;

    };
    

    class FC_P_EI : public FullConditional
    {
        public:
            FC_P_EI(ModelContext *_context,
                    CompartmentalModelMatrix *_I_star,
                    CompartmentalModelMatrix *_E_star,
                    InitData *_A0,
                    double *_p_ei);
            virtual int evalCPU();
            virtual int evalOCL();
            virtual int sampleCPU();
            virtual int sampleOCL();
            ~FC_P_EI();
            ModelContext** context;
            CompartmentalModelMatrix **I_star;
            CompartmentalModelMatrix **E_star;
            InitData **A0;
            double **p_ei;
            double* value;
    };

    class FC_P_IR : public FullConditional 
    {
        
        public:
            FC_P_IR(ModelContext *_context,
                    CompartmentalModelMatrix *_I_star, 
                    CompartmentalModelMatrix *_R_star,
                    InitData *_A0,
                    double *_p_ir);
            ~FC_P_IR();
            virtual int evalCPU();
            virtual int evalOCL();
            virtual int sampleCPU();
            virtual int sampleOCL();
            ModelContext **context;
            CompartmentalModelMatrix **I_star;
            CompartmentalModelMatrix **R_star;
            InitData **A0;
            double **p_ir;
            double* value;
    };

}
