//-----------------------------------------------------------------------------
// Entaro ChucK Developer!
// This is a Chugin boilerplate, generated by chuginate!
//-----------------------------------------------------------------------------

// this should align with the correct versions of these ChucK files
#include "chuck_dl.h"
#include "chuck_def.h"
#include "chuck_instr.h"
#include "chuck_vm.h"

// general includes
#include <stdio.h>
#include <limits.h>

// STL includes
#include <iostream>
#include <string>
using namespace std;

#include "JuceHeader.h"


// declaration of chugin constructor
CK_DLL_CTOR(vst_ctor);
// declaration of chugin desctructor
CK_DLL_DTOR(vst_dtor);

// functions
CK_DLL_MFUN(vst_loadPlugin);
CK_DLL_MFUN(vst_getnumparameters);
CK_DLL_MFUN(vst_getparametername);
CK_DLL_MFUN(vst_getparameter);
CK_DLL_MFUN(vst_setparameter);
CK_DLL_MFUN(vst_noteon);
CK_DLL_MFUN(vst_noteoff);
CK_DLL_MFUN(vst_loadPreset);

// multi-channel audio synthesis tick function
CK_DLL_TICKF(vst_tickstereoin);

// this is a special offset reserved for Chugin internal data
t_CKINT vst_data_offset = 0;


//-----------------------------------------------------------------------------
// name: class VST
// desc: VST instrument/effect implementation (via JUCE)
//-----------------------------------------------------------------------------
class VST
{
public:
    // constructor
    VST(t_CKFLOAT srate)
    {
        // sample rate
        m_srate = srate;

        // output is stereo
        m_chans = 2;

        myBuffer = new juce::AudioBuffer<float>(m_chans, 512);
    }

    ~VST()
    {
        if (myPlugin)
        {
            myPlugin->releaseResources();
        }
    
        if (myBuffer) {
            delete myBuffer;
        }

        delete myChuckString;
    }

    // for Chugins extending UGen
    void tickstereoin(SAMPLE* in, SAMPLE* out, int nframes);
    
    bool loadPlugin(const string& filename);
    bool loadPreset(const string& filename);

    bool noteOn(int note, float velocity);
    bool noteOff(int note, float velocity);

    int getNumParameters();
    Chuck_String* getParameterName(int index);
    float getParameter(int index);
    bool setParameter(int index, float v);

private:
    // data
    t_CKINT m_chans;

    // sample rate
    t_CKFLOAT m_srate;

protected:
    
    juce::AudioPluginInstance* myPlugin = nullptr;

    std::string myPluginPath = "";

    juce::AudioSampleBuffer* myBuffer;
    juce::MidiBuffer myMidiBuffer;
    juce::MidiBuffer myRenderMidiBuffer;
    Chuck_String* myChuckString = new Chuck_String("");

};

void VST::tickstereoin(SAMPLE* in, SAMPLE* out, int nframes)
{

    if (!myPlugin) {

        // for now, set to zeros.
        for (int i = 0; i < nframes; i++)
        {
            out[i * 2] = 0;
            out[i * 2 + 1] = 0;
        }

        return;
    }

    myBuffer->setSize(2, nframes, false, true, false);
    // in contains alternating left and right channels.
    for (int chan = 0; chan < 2; chan++) {
        auto chanPtr = myBuffer->getWritePointer(chan);
        for (int i = 0; i < nframes; i++)
        {
            *chanPtr++ = in[i * 2 + chan];
        }
    }

    myPlugin->processBlock(*myBuffer, myMidiBuffer);

    // copy from buffer to output.
    // out needs to receive alternating left/right channels.
    for (int chan = 0; chan < 2; chan++) {
        auto chanPtr = myBuffer->getReadPointer(chan);
        for (int i = 0; i < nframes; i++)
        {
            out[chan + 2 * i] = *chanPtr++;
        }
    }
}


//-----------------------------------------------------------------------------
// name: loadPlugin()
// desc: load VST Plugin
//-----------------------------------------------------------------------------
bool VST::loadPlugin(const string& filename)
{
    using namespace juce;

    OwnedArray<PluginDescription> pluginDescriptions;
    KnownPluginList pluginList;
    AudioPluginFormatManager pluginFormatManager;

    pluginFormatManager.addDefaultFormats();

    for (int i = pluginFormatManager.getNumFormats(); --i >= 0;)
    {
        pluginList.scanAndAddFile(String(filename),
            true,
            pluginDescriptions,
            *pluginFormatManager.getFormat(i));
    }

    // If there is a problem here first check the preprocessor definitions
    // in the projucer are sensible - is it set up to scan for plugin's?
    if (pluginDescriptions.size() == 0) {
        return false;
    }

    String errorMessage;

    if (myPlugin)
    {
        myPlugin->releaseResources();
    }

    int samplesPerBlock = 512;  // todo(DBraun): is 512 right?
    float sampleRate = m_srate;

    myPlugin = pluginFormatManager.createPluginInstance(*pluginDescriptions[0],
        sampleRate,
        samplesPerBlock,
        errorMessage);

    if (myPlugin != nullptr)
    {
        // Success so set up plugin, then set up features and get all available
        // parameters from this given plugin.
        myPlugin->prepareToPlay(sampleRate, samplesPerBlock);
        myPlugin->setNonRealtime(false);

        myPluginPath = filename;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// name: loadPreset()
// desc: load VST preset
//-----------------------------------------------------------------------------
bool VST::loadPreset(const string& filename)
{
    using namespace juce;

    try
    {
        if (!myPlugin) {
            return false;
        }

        MemoryBlock mb;
        File file = File(filename);
        file.loadFileAsData(mb);

#if JUCE_PLUGINHOST_VST
        // The VST2 way of loading preset. You need the entire VST2 SDK source, which is not public.
        VSTPluginFormat::loadFromFXBFile(myPlugin, mb.getData(), mb.getSize());
#else
        return false;
#endif
    }
    catch (const std::exception&)
    {
        return false;
    }

    return true;

}

bool VST::noteOn(int noteNumber, float velocity) {
    using namespace juce;
    
    // Get the note on midiBuffer.
    MidiMessage onMessage = MidiMessage::noteOn(1, noteNumber, velocity);
    onMessage.setTimeStamp(0);
    myMidiBuffer.addEvent(onMessage, onMessage.getTimeStamp());

    return true;
}

bool VST::noteOff(int noteNumber, float velocity) {
    using namespace juce;

    MidiMessage offMessage = MidiMessage::noteOff(1, noteNumber, velocity);
    offMessage.setTimeStamp(0);
    myMidiBuffer.addEvent(offMessage, offMessage.getTimeStamp());
    
    return true;
}

int VST::getNumParameters() {

    if (!myPlugin) {
        return 0;
    }
    return myPlugin->getNumParameters();
}

Chuck_String* VST::getParameterName(int index) {

    if (!myPlugin) {
        myChuckString->set("");
        return myChuckString;
    }

    try
    {
        myChuckString->set(myPlugin->getParameterName(index).toStdString());
        return myChuckString;
    }
    catch (const std::exception&)
    {
        myChuckString->set("");
        return myChuckString;
    }
}

float VST::getParameter(int index) {

    if (!myPlugin) {
        return 0.;
    }

    try
    {
        return myPlugin->getParameter(index);
    }
    catch (const std::exception&)
    {
        return 0.;
    }
}

bool VST::setParameter(int index, float v) {

    if (!myPlugin) {
        return false;
    }

    try
    {
        myPlugin->setParameter(index, v);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}


//-----------------------------------------------------------------------------
// query function: chuck calls this when loading the Chugin
//-----------------------------------------------------------------------------
CK_DLL_QUERY( VST )
{
    // hmm, don't change this...
    QUERY->setname(QUERY, "VST");

    // begin the class definition
    // can change the second argument to extend a different ChucK class
    QUERY->begin_class(QUERY, "VST", "UGen");

    // register the constructor (probably no need to change)
    QUERY->add_ctor(QUERY, vst_ctor);
    // register the destructor (probably no need to change)
    QUERY->add_dtor(QUERY, vst_dtor);

    // 2, 2 to do stereo in and stereo out
    QUERY->add_ugen_funcf(QUERY, vst_tickstereoin, NULL, 2, 2);

    QUERY->add_mfun(QUERY, vst_loadPlugin, "int", "loadPlugin");
    QUERY->add_arg(QUERY, "string", "filename");

    QUERY->add_mfun(QUERY, vst_loadPreset, "int", "loadPreset");
    QUERY->add_arg(QUERY, "string", "filename");

    QUERY->add_mfun(QUERY, vst_getnumparameters, "int", "getNumParameters");

    QUERY->add_mfun(QUERY, vst_getparametername, "string", "getParameterName");
    QUERY->add_arg(QUERY, "int", "index");

    QUERY->add_mfun(QUERY, vst_getparameter, "float", "getParameter");
    QUERY->add_arg(QUERY, "int", "index");

    QUERY->add_mfun(QUERY, vst_setparameter, "int", "setParameter");
    QUERY->add_arg(QUERY, "int", "index");
    QUERY->add_arg(QUERY, "float", "value");

    QUERY->add_mfun(QUERY, vst_noteon, "int", "noteOn");
    QUERY->add_arg(QUERY, "int", "noteNumber");
    QUERY->add_arg(QUERY, "float", "velocity");

    QUERY->add_mfun(QUERY, vst_noteoff, "int", "noteOff");
    QUERY->add_arg(QUERY, "int", "noteNumber");
    QUERY->add_arg(QUERY, "float", "velocity");

    // this reserves a variable in the ChucK internal class to store
    // referene to the c++ class we defined above
    vst_data_offset = QUERY->add_mvar(QUERY, "int", "@b_data", false);

    // end the class definition
    // IMPORTANT: this MUST be called!
    QUERY->end_class(QUERY);

    // wasn't that a breeze?
    return TRUE;
}

// implementation for the constructor
CK_DLL_CTOR(vst_ctor)
{
    // get the offset where we'll store our internal c++ class pointer
    OBJ_MEMBER_INT(SELF, vst_data_offset) = 0;

    // instantiate our internal c++ class representation
    VST * b_obj = new VST(API->vm->get_srate(API, SHRED));

    // store the pointer in the ChucK object member
    OBJ_MEMBER_INT(SELF, vst_data_offset) = (t_CKINT) b_obj;
}

// implementation for the destructor
CK_DLL_DTOR(vst_dtor)
{
    // get our c++ class pointer
    VST * b_obj = (VST *) OBJ_MEMBER_INT(SELF, vst_data_offset);
    // check it
    if( b_obj )
    {
        // clean up
        delete b_obj;
        OBJ_MEMBER_INT(SELF, vst_data_offset) = 0;
        b_obj = NULL;
    }
}

// implementation for tick function
CK_DLL_TICKF(vst_tickstereoin)
{
    // get our c++ class pointer
    VST* b = (VST*)OBJ_MEMBER_INT(SELF, vst_data_offset);

    // invoke our tick function; store in the magical out variable
    if (b) b->tickstereoin(in, out, nframes);

    // yes
    return TRUE;
}

CK_DLL_MFUN(vst_loadPlugin)
{
    string filename = GET_NEXT_STRING_SAFE(ARGS);

    VST* b = (VST*)OBJ_MEMBER_INT(SELF, vst_data_offset);

    RETURN->v_int = b->loadPlugin(filename);
}

CK_DLL_MFUN(vst_loadPreset)
{
    string filename = GET_NEXT_STRING_SAFE(ARGS);

    VST* b = (VST*)OBJ_MEMBER_INT(SELF, vst_data_offset);

    RETURN->v_int = b->loadPreset(filename);
}

CK_DLL_MFUN(vst_getnumparameters)
{
    VST* b = (VST*)OBJ_MEMBER_INT(SELF, vst_data_offset);

    RETURN->v_int = b->getNumParameters();
}

CK_DLL_MFUN(vst_getparametername)
{
    t_CKINT index = GET_NEXT_INT(ARGS);

    VST* b = (VST*)OBJ_MEMBER_INT(SELF, vst_data_offset);

    RETURN->v_string = b->getParameterName(index);
}

CK_DLL_MFUN(vst_getparameter)
{
    t_CKINT index = GET_NEXT_INT(ARGS);

    VST* b = (VST*)OBJ_MEMBER_INT(SELF, vst_data_offset);

    RETURN->v_float = b->getParameter(index);
}

CK_DLL_MFUN(vst_setparameter)
{
    t_CKINT index = GET_NEXT_INT(ARGS);
    t_CKFLOAT val = GET_NEXT_FLOAT(ARGS);

    VST* b = (VST*)OBJ_MEMBER_INT(SELF, vst_data_offset);

    RETURN->v_int = b->setParameter(index, val);
}

CK_DLL_MFUN(vst_noteon)
{
    t_CKINT noteNumber = GET_NEXT_INT(ARGS);
    t_CKFLOAT velocity = GET_NEXT_FLOAT(ARGS);

    VST* b = (VST*)OBJ_MEMBER_INT(SELF, vst_data_offset);

    RETURN->v_int = b->noteOn(noteNumber, velocity);
}

CK_DLL_MFUN(vst_noteoff)
{
    t_CKINT noteNumber = GET_NEXT_INT(ARGS);
    t_CKFLOAT velocity = GET_NEXT_FLOAT(ARGS);

    VST* b = (VST*)OBJ_MEMBER_INT(SELF, vst_data_offset);

    RETURN->v_int = b->noteOff(noteNumber, velocity);
}
