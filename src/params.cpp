// params.cpp
// See params.h for notes.

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <cctype>
#include <cstdint>
#include <map>
#include "params.h"

#define MAX_FPS 100.0

// To add a new parameter:
//    1. Add a member to struct Params in params.h.
//    2. Add a member and its default value to privParams in ParamManager::setDefaults()
//          in params.cpp.
//    3. Add an else clause to ParamManager::ingestParameter() in params.cpp.
//    4. Add a line to the user's parameter file (default name biosim4.ini)


std::ofstream sampleGenomeOutfile;

void ParamManager::setDefaults()
{
    privParams.sizeX = 128;
    privParams.sizeY = 128;
    privParams.challenge = 6;

    privParams.genomeInitialLengthMin = 24;
    privParams.genomeInitialLengthMax = 24;
    privParams.genomeMaxLength = 300;
    privParams.logDir = "./logs/";
    privParams.imageDir = "./images/";
    privParams.population = 3000;
    privParams.stepsPerGeneration = 300;
    privParams.maxGenerations = 200000;
    privParams.barrierType = 0;
    privParams.replaceBarrierType = 0;
    privParams.replaceBarrierTypeGenerationNumber = (uint32_t)-1;
    privParams.numThreads = 4;
    privParams.signalLayers = 1;
    privParams.maxNumberNeurons = 5;
    privParams.pointMutationRate = 0.001;
    privParams.geneInsertionDeletionRate = 0.0;
    privParams.deletionRatio = 0.5;
    privParams.killEnable = false;
    privParams.sexualReproduction = true;
    privParams.chooseParentsByFitness = true;
    privParams.populationSensorRadius = 2.5;
    privParams.signalSensorRadius = 2.0;
    privParams.responsiveness = 0.5;
    privParams.responsivenessCurveKFactor = 2;
    privParams.longProbeDistance = 16;
    privParams.shortProbeBarrierDistance = 4;
    privParams.valenceSaturationMag = 0.5;
    privParams.saveVideo = true;
    privParams.videoStride = 25;
    privParams.videoSaveFirstFrames = 2;
    privParams.displayScale = 8;
    privParams.agentSize = 4;
    privParams.genomeAnalysisStride = privParams.videoStride;
    privParams.displaySampleGenomes = 5;
    privParams.genomeComparisonMethod = 1;
    privParams.updateGraphLog = true;
    privParams.updateGraphLogStride = privParams.videoStride;
    privParams.deterministic = false;
    privParams.RNGSeed = 12345678;
    privParams.graphLogUpdateCommand = "/usr/bin/gnuplot --persist ./tools/graphlog.gp";
    sampleGenomeOutfile.open("./tools/nnet.txt", std::ofstream::out);
    privParams.videoFPS = 25.0;
}


void ParamManager::registerConfigFile(const char *filename)
{
    configFilename = std::string(filename);
}


bool checkIfUint(const std::string &s)
{
    return s.find_first_not_of("0123456789") == std::string::npos;
}


bool checkIfInt(const std::string &s)
{
    //return s.find_first_not_of("-0123456789") == std::string::npos;
    std::istringstream iss(s);
    int i;
    iss >> std::noskipws >> i; // noskipws considers leading whitespace invalid
    // Check the entire string was consumed and if either failbit or badbit is set
    return iss.eof() && !iss.fail();
}


bool checkIfFloat(const std::string &s)
{
    std::istringstream iss(s);
    double d;
    iss >> std::noskipws >> d; // noskipws considers leading whitespace invalid
    // Check the entire string was consumed and if either failbit or badbit is set
    return iss.eof() && !iss.fail();
}


bool checkIfBool(const std::string &s)
{
    return s == "0" || s == "1" || s == "true" || s == "false";
}


bool getBoolVal(const std::string &s)
{
    if (s == "true" || s == "1")
        return true;
    else if (s == "false" || s == "0")
        return false;
    else
        return false;
}


void ParamManager::ingestParameter(std::string name, std::string val)
{
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    //std::cout << name << " " << val << '\n' << std::endl;

    bool isUint = checkIfUint(val);
    unsigned uVal = isUint ? (unsigned)std::stol(val.c_str()) : 0;
    bool isInt = checkIfInt(val);
    int iVal = isInt ? std::stoi(val.c_str()) : 0;
    bool isFloat = checkIfFloat(val);
    double dVal = isFloat ? std::stod(val.c_str()) : 0.0;
    bool isBool = checkIfBool(val);
    bool bVal = getBoolVal(val);

    if (name == "sizex" && isUint && uVal >= 2 && uVal <= (uint16_t)-1) {
        privParams.sizeX = uVal; 
    } else if (name == "sizey" && isUint && uVal >= 2 && uVal <= (uint16_t)-1) {
        privParams.sizeY = uVal; 
    } else if (name == "videofps" && isFloat && dVal >= 1.0) {
        privParams.videoFPS = dVal <= MAX_FPS ? dVal : MAX_FPS;
    } else if (name == "challenge" && isUint && uVal < (uint16_t)-1) {
        privParams.challenge = uVal; 
    } else if (name == "genomeinitiallengthmin" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
        privParams.genomeInitialLengthMin = uVal; 
    } else if (name == "genomeinitiallengthmax" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
        privParams.genomeInitialLengthMax = uVal; 
    } else if (name == "logdir") {
        privParams.logDir = val; 
    } else if (name == "imagedir") {
        privParams.imageDir = val; 
    } else if (name == "population" && isUint && uVal > 0 && uVal < (uint32_t)-1) {
        privParams.population = uVal; 
    } else if (name == "stepspergeneration" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
        privParams.stepsPerGeneration = uVal; 
    } else if (name == "maxgenerations" && isUint && uVal > 0 && uVal < 0x7fffffff) {
        privParams.maxGenerations = uVal; 
    } else if (name == "barriertype" && isUint && uVal < (uint32_t)-1) {
        privParams.barrierType = uVal; 
    } else if (name == "replacebarriertype" && isUint && uVal < (uint32_t)-1) {
        privParams.replaceBarrierType = uVal; 
    } else if (name == "replacebarriertypegenerationnumber" && isInt && iVal >= -1) {
        privParams.replaceBarrierTypeGenerationNumber = (iVal == -1 ? (uint32_t)-1 : iVal); 
    } else if (name == "numthreads" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
        privParams.numThreads = uVal; 
    } else if (name == "signallayers" && isUint && uVal < (uint16_t)-1) {
        privParams.signalLayers = uVal; 
    } else if (name == "genomemaxlength" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
        privParams.genomeMaxLength = uVal; 
    } else if (name == "maxnumberneurons" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
        privParams.maxNumberNeurons = uVal; 
    } else if (name == "pointmutationrate" && isFloat && dVal >= 0.0 && dVal <= 1.0) {
        privParams.pointMutationRate = dVal; 
    } else if (name == "geneinsertiondeletionrate" && isFloat && dVal >= 0.0 && dVal <= 1.0) {
        privParams.geneInsertionDeletionRate = dVal; 
    } else if (name == "deletionratio" && isFloat && dVal >= 0.0 && dVal <= 1.0) {
        privParams.deletionRatio = dVal; 
    } else if (name == "killenable" && isBool) {
        privParams.killEnable = bVal; 
    } else if (name == "sexualreproduction" && isBool) {
        privParams.sexualReproduction = bVal; 
    } else if (name == "chooseparentsbyfitness" && isBool) {
        privParams.chooseParentsByFitness = bVal; 
    } else if (name == "populationsensorradius" && isFloat && dVal > 0.0) {
        privParams.populationSensorRadius = dVal; 
    } else if (name == "signalsensorradius" && isFloat && dVal > 0.0) {
        privParams.signalSensorRadius = dVal; 
    } else if (name == "responsiveness" && isFloat && dVal >= 0.0) {
        privParams.responsiveness = dVal; 
    } else if (name == "responsivenesscurvekfactor" && isUint && uVal >= 1 && uVal <= 20) {
        privParams.responsivenessCurveKFactor = uVal; 
    } else if (name == "longprobedistance" && isUint && uVal > 0) {
        privParams.longProbeDistance = uVal; 
    } else if (name == "shortprobebarrierdistance" && isUint && uVal > 0) {
        privParams.shortProbeBarrierDistance = uVal; 
    } else if (name == "valencesaturationmag" && isFloat && dVal >= 0.0) {
        privParams.valenceSaturationMag = dVal; 
    } else if (name == "savevideo" && isBool) {
        privParams.saveVideo = bVal; 
    } else if (name == "videostride" && isUint && uVal > 0) {
        privParams.videoStride = uVal; 
    } else if (name == "videosavefirstframes" && isUint) {
        privParams.videoSaveFirstFrames = uVal; 
    } else if (name == "displayscale" && isUint && uVal > 0) {
        privParams.displayScale = uVal; 
    } else if (name == "agentsize" && isFloat && dVal > 0.0) {
        privParams.agentSize = dVal; 
    } else if (name == "genomeanalysisstride" && isUint && uVal > 0) {
        privParams.genomeAnalysisStride = uVal; 
    } else if (name == "genomeanalysisstride" && val == "videoStride") {
        privParams.genomeAnalysisStride = privParams.videoStride; 
    } else if (name == "displaysamplegenomes" && isUint) {
        privParams.displaySampleGenomes = uVal; 
    } else if (name == "genomecomparisonmethod" && isUint) {
        privParams.genomeComparisonMethod = uVal; 
    } else if (name == "updategraphlog" && isBool) {
        privParams.updateGraphLog = bVal; 
    } else if (name == "updategraphlogstride" && isUint && uVal > 0) {
        privParams.updateGraphLogStride = uVal; 
    } else if (name == "updategraphlogstride" && val == "videoStride") {
        privParams.updateGraphLogStride = privParams.videoStride; 
    } else if (name == "deterministic" && isBool) {
        privParams.deterministic = bVal; 
    } else if (name == "rngseed" && isUint) {
        privParams.RNGSeed = uVal; 
    } else if (name == "samplegenomeoutfile") {
        sampleGenomeOutfile.open(val, std::ofstream::out);
    } else {
        std::cout << "Invalid param: " << name << " = " << val << std::endl;
    }
}

void ParamManager::updateFromConfigFile() {
    // std::ifstream is RAII, i.e. no need to call close
    std::ifstream cFile(configFilename.c_str());
    if (cFile.is_open()) {
        std::string line;
        while(getline(cFile, line)){
            line.erase(std::remove_if(line.begin(), line.end(), isspace),
                                 line.end());
            if(line[0] == '#' || line.empty()) {
                continue;
            }
            auto delimiterPos = line.find("=");
            auto name = line.substr(0, delimiterPos);
            std::transform(name.begin(), name.end(), name.begin(),
                        [](unsigned char c){ return std::tolower(c); });
            auto value0 = line.substr(delimiterPos + 1);
            auto delimiterComment = value0.find("#");
            auto value = value0.substr(0, delimiterComment);
            auto rawValue = value;
            value.erase(std::remove_if(value.begin(), value.end(), isspace),
                                 value.end());
            //std::cout << name << " " << value << '\n' << std::endl;
            ingestParameter(name, value);
        }
    }
    else {
        std::cerr << "Couldn't open config file " << configFilename << ".\n" << std::endl;
    }
}


// Check parameter ranges, reasonableness, coherency, whatever. This is
// typically called only once after the parameters are first read.
void ParamManager::checkParameters()
{
    if (privParams.deterministic && privParams.numThreads != 1) {
        std::cerr << "Warning: When deterministic is true, you probably want to set numThreads = 1." << std::endl;
    }
}



