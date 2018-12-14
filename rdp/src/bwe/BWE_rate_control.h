#ifndef BWE_RATE_CONTROL_H
#define BWE_RATE_CONTROL_H
#include <iostream>
#include <list>
/*
BWE_rate_control.h is a combination of Overuse_detecter.h and Remote_rate_control.h.

class RemoteRateControl's function for external use is
I)     Update (update remote rate control region -- kRcNearMax, kRcAboveMax, kRcMaxUnknown)
II)    SetCpuLimitedRemoteBR (abandoned)
III)   UpdateLossMetric (update packet loss information)
IV)   UpdateBandwidthEstimate (update bandwidth estimation)
V)    Reset(reset parameters)
VI)   SetAsufficBitRate(set asuffic bit rate)
VII)  LatestEstimate(get bandwidth estimation)

class RemoteRateControl adds function for RUDP is
I)    UpdatePacket(update packet information)
II)   UpdateBandwidthEstimate (update bandwidth estimation)

class OverUseDetecter's function for external use is
I)    Reset(reset parameters)
II)   Update(update network state)
III)  SetRateControlRegion(set remote rate control region -- kRcNearMax, kRcAboveMax, kRcMaxUnknown)
IV)  State(get network state -- kBwNormal, kBwOverusing or kBwUnderUsing)
V)    NoiseVar(get noise Variance)
*/
namespace bwe
{

#define BWE_MAX(a,b) ((a)>(b)?(a):(b))
#define BWE_MIN(a,b) ((a)<(b)?(a):(b))

     // Define C99 equivalent types, since MSVC doesn't provide stdint.h.
    typedef signed char         int8_t;
    typedef signed short        int16_t;
    typedef signed int          int32_t;
    typedef long long           int64_t;
    typedef unsigned char       uint8_t;
    typedef unsigned short      uint16_t;
    typedef unsigned int        uint32_t;
    typedef unsigned long long  uint64_t;
    typedef int8_t              WebRtc_Word8;
    typedef int16_t             WebRtc_Word16;
    typedef int32_t             WebRtc_Word32;
    typedef int64_t             WebRtc_Word64;
    typedef uint8_t             WebRtc_UWord8;
    typedef uint16_t            WebRtc_UWord16;
    typedef uint32_t            WebRtc_UWord32;
    typedef uint64_t            WebRtc_UWord64;

    enum { kBwReportInterval = 1000};
    enum { kBwThVeryLow = 50000,
        kBwThLow = 100000,
        kBwThNormal = 200000,
        kBwThGood = 400000,
        kBwThVeryGood = 800000};

    //network bw status
    enum NetwBandwidthStatus {
        kNetworkBwUnknown = 0,
        kNetworkBwVeryLow,
        kNetworkBwLow,
        kNetworkBwNormal,
        kNetworkBwGood,
        kNetworkBwVeryGood,
    };

    enum BandwidthUsage
    {
        kBwNormal,
        kBwOverusing,
        kBwUnderUsing
    };

    enum RateControlRegion
    {
        kRcNearMax,
        kRcAboveMax,
        kRcMaxUnknown
    };

    enum RateControlState
    {
        kRcHold,
        kRcIncrease,
        kRcDecrease
    };

    class RateControlInputEx
    {
    public:
        RateControlInputEx(WebRtc_UWord64 timeStamp, WebRtc_UWord64 sequenceNumber, WebRtc_UWord16 packetSize, WebRtc_UWord64 nowMS, WebRtc_Word32 incomingBitRate) :
        _timestamp(timeStamp), _sequenceNumber(sequenceNumber), _packetSize(packetSize), _nowMS(nowMS), _incomingBitRate(incomingBitRate)
        {};
		WebRtc_UWord64  _timestamp;
		WebRtc_UWord64  _sequenceNumber;
		WebRtc_UWord16  _packetSize;
		WebRtc_UWord64  _nowMS;
		WebRtc_Word32   _noiseVar;
		WebRtc_Word32   _incomingBitRate;
		BandwidthUsage  _bwState;
    };

	class RateControlInput
    {
    public:
        RateControlInput(BandwidthUsage bwState,
            WebRtc_UWord32 incomingBitRate,
            double noiseVar) :
        _bwState(bwState), _incomingBitRate(incomingBitRate), _noiseVar(noiseVar)
        {};

        BandwidthUsage  _bwState;
        WebRtc_UWord32  _incomingBitRate;
        double          _noiseVar;
    };
	
    class OverUseDetector
    {
    public:
        OverUseDetector();
        ~OverUseDetector();
        bool Update(const WebRtc_UWord64 timestamp, const WebRtc_UWord64 sequenceNumber,
            const WebRtc_UWord16 packetSize,
            const WebRtc_Word64 nowMS);
        BandwidthUsage State() const;
        void Reset();
        double NoiseVar() const;
        void SetRateControlRegion(RateControlRegion region);
    private:

		enum { kDeltaCounterMax = 1000 };
		enum { kMinFramePeriodHistoryLength = 60 };
		
        struct FrameSample
        {
            FrameSample() : _size(0), _completeTimeMs(-1), _timestamp(-1) {}

            WebRtc_UWord32 _size;
            WebRtc_Word64  _completeTimeMs;
            WebRtc_Word64  _timestamp;
        };

        void CompensatedTimeDelta(const FrameSample& currentFrame,
            const FrameSample& prevFrame,
            WebRtc_Word64& tDelta,
            double& tsDelta,
            bool wrapped);
        void UpdateKalman(WebRtc_Word64 tDelta,
            double tsDelta,
            WebRtc_UWord32 frameSize,
            WebRtc_UWord32 prevFrameSize);

        void SimpleDetect(double tTsDelta, double frameSizeDelta, 
            const WebRtc_Word64 nowMS);

        double UpdateMinFramePeriod(double tsDelta);
        void UpdateNoiseEstimate(double residual, double tsDelta, bool stableState);
        BandwidthUsage Detect(double tsDelta);
		BandwidthUsage DetectEx(double ts_delta);
        double CurrentDrift();
        bool OldSequence(WebRtc_UWord16 newSeq, WebRtc_UWord32 existingSeq);
        bool OldTimestamp(uint32_t newTimestamp, uint32_t existingTimestamp, bool* wrapped);

        void UpdateKalmanEx(int64_t t_delta, double ts_delta, int size_delta, BandwidthUsage current_hypothesis);
        double UpdateMinFramePeriodEx(double ts_delta);
		void UpdateNoiseEstimateEx(double residual, double ts_delta, bool stable_state);

        bool _firstPacket;
        FrameSample _currentFrame;
        FrameSample _prevFrame;
        WebRtc_UWord16 _numOfDeltas;
        double _slope;
        double _offset;
        double _E[2][2];
        double _processNoise[2];
        double _avgNoise;
        double _varNoise;
        double _threshold;
        std::list<double> _tsDeltaHist;
        double _prevOffset;
        double _timeOverUsing;
        WebRtc_UWord16 _overUseCounter;
        BandwidthUsage _hypothesis;
        WebRtc_UWord16 _lastSeqNo;
        WebRtc_Word64 _lastUpdateTimeMS;
        double _tTsDeltaJitter;
        double _posiFrameDelta;
        double _posiTsDelta;
    };

    class SendRateControl
    {
    public:
		SendRateControl();
		~SendRateControl();
        WebRtc_UWord32 ShapeSimple(WebRtc_Word32 packetLoss, WebRtc_Word32 rtt,
                         WebRtc_UWord32 sentBitrate, WebRtc_Word64 nowMS);
		WebRtc_UWord32 ShapeSimpleRtt(WebRtc_Word32 rtt, WebRtc_UWord32 sentBitrate, float timeInterval);
		WebRtc_Word32  CalcTFRCbps(WebRtc_Word16 avgPackSizeBytes, WebRtc_Word32 rttMs,
                         WebRtc_Word32 packetLoss);
		
	private:
		WebRtc_UWord32        _bitRate;
		WebRtc_Word64         _timeLastChange;
		double                _avgLoss;
		WebRtc_Word64         _timeAccumulateLoss;
		
    };
    class RemoteRateControl
    {
    public:
        RemoteRateControl();
		
        ~RemoteRateControl();
        WebRtc_Word32 SetConfiguredBitRates(WebRtc_UWord32 minBitRate,
            WebRtc_UWord32 maxBitRate);
        WebRtc_UWord32 LatestEstimate() const;
        WebRtc_UWord32 UpdateBandwidthEstimate(WebRtc_UWord32 RTT,
            WebRtc_Word64 nowMS, float remoteTargetBrRatio, bool rtcpTimeout = false);
        void Reset();

        // Returns true if there is a valid estimate of the incoming bitrate, false
        // otherwise.
        bool ValidEstimate() const;
        RateControlRegion Update(const RateControlInput& input, bool& firstOverUse,
            WebRtc_Word64 nowMS,WebRtc_UWord32 tmmbnRatekbit=0);
        void UpdatePacket(const WebRtc_UWord64 timestamp, const WebRtc_UWord64 sequenceNumber,
            const WebRtc_UWord16 packetSize, const WebRtc_Word64 nowMS);
        void UpdateLossMetric(WebRtc_UWord32 loss,
            WebRtc_UWord32 accumLoss,WebRtc_UWord32 maxSeq, WebRtc_UWord32 nowMs);

        //other module(sdp or rtcp) can set this parm, need to notice the critical section!
        void SetAsufficBitRate(WebRtc_UWord32 asufficBitRate);
        //cpu limited remote bitrate set by media_opt based on cpu load control module
        // percent: if set, effective only once; limit next tmmbr by percent(0-1)
        // maxBrKbit: if set, effective throughout the session
        void SetCpuLimitedRemoteBR(float percent,WebRtc_UWord32 minBrKbit,WebRtc_UWord32 maxBrKbit);
		NetwBandwidthStatus GetBandWidthStatus() const;

    private:
        WebRtc_UWord32 ChangeBitRate(WebRtc_UWord32 currentBitRate,
            WebRtc_UWord32 incomingBitRate,
            double delayFactor, WebRtc_UWord32 RTT,
            WebRtc_Word64 nowMS,
            float remoteTargetBrRatio);
		void  ProcessBitRate(const WebRtc_Word64 nowMS);

        double CalcRateIncrease(WebRtc_UWord32 currentBitRate, 
            WebRtc_UWord32 reactionTimeMs, float incommingRatio) const;

        void UpdateOveruseRate(WebRtc_UWord32 incomingBitRate, RateControlState curState);
        void UpdateRTT(WebRtc_UWord32 RTT);

        void UpdateMaxBitRateEstimate(float incomingBitRateKbps);
        void ChangeState(const RateControlInput& input, WebRtc_Word64 nowMs);
        void ChangeState(RateControlState newState);
        void ChangeRegion(RateControlRegion region);
        static void StateStr(RateControlState state, char* str);
        static void StateStr(BandwidthUsage state, char* str);
        void UpdateDetection(WebRtc_UWord32 incomingBitRate, double noiseVar, BandwidthUsage  bwState);

        enum {kUpdateLossPacketInterval = 255};
        enum {kUpdateLossTimeInterval = 2000};
        enum {kMinPacketInterval = 20};
        enum {kMinTimeInterval = 1000};
        enum {kPacketThreshold = 500};
        enum {kLossOveruseHardlimit = 50}; //50 in 255, avg loss >20% loss will be categorized into overuse
        enum {kLossOveruseSoftlimit = 5}; //5 in 255, avg loss <2% will be categorized into normal
        enum {kOveruseMeasureWindow = 1500}; //average overuse rate per frames window

        WebRtc_UWord32      _minConfiguredBitRate;
        WebRtc_UWord32      _maxConfiguredBitRate;
        WebRtc_UWord32      _currentBitRate;
        WebRtc_UWord32      _maxHoldRate;
        float               _avgMaxBitRateKbps;
        float               _varMaxBitRateKbps;
        RateControlState    _rcState;
        RateControlState    _cameFromState;
        RateControlRegion   _rcRegion;
        WebRtc_Word64       _lastBitRateChange;
        RateControlInput    _currentInput;
        bool                _updated;
		bool                _firstOverUse;

        WebRtc_Word64       _lastChangeMs;
        WebRtc_UWord32      _lastChangeBitrate;
        float               _beta;
        bool                _initializedBitRate;
        WebRtc_Word64       _timeFirstIncomingEstimate;
        double              _overUseRate;  
        double              _lloverUseRate;  //long long time(more than 10 minutes) overuse rate statics
        //todo: consider to save the network learning data to persistent.

        WebRtc_UWord32      _asufficBitRate; //Appropriate sufficient bitrate
        //assume other module(sdp or rtcp) can set this parm

		NetwBandwidthStatus _state; //bandwidth status

        // this is one time effective remote bitrate percent
        // it is set when decoding consumes too much cpu power
        // limit sent tmmbr value to lower incoming stream throughput
        // in percent(0-1)
        // if _minCpuLimitedBr is set, dont' lower tmmbr value than it;
        float               _bitratePercent;
        WebRtc_UWord32      _minCpuLimitedBr;

        WebRtc_UWord32      _meanOverUseBr;
        float               _avgRTTFar;
        float               _avgRTTNear;
        WebRtc_UWord32      _rttThreshold;

        WebRtc_UWord32      _initIncomingRate;

        //loss
        WebRtc_UWord32      _avgLoss; //average loss: loss caused by link failure and loss caused by congestion
        WebRtc_UWord32      _varLoss;
        BandwidthUsage      _lossHypothesis;
        WebRtc_UWord32      _prevAccumLoss;
        WebRtc_UWord32      _prevMaxSeqNo;
        WebRtc_UWord32      _prevTime;
        WebRtc_UWord32      _numUpdate;
        WebRtc_UWord32      _overusePacketCount;
        bool                _enableLossMetric;
        WebRtc_UWord32      _baseLoss; //loss due to phisical link failure
        WebRtc_UWord32      _normalCount;
        WebRtc_Word32       _lastTimeChangeRate; //last time drop bitrate due to overuse detected by loss

        //network monitor
        WebRtc_Word64       _timeLastBwReport;
        bool                _initialState;

		//process bitrate
		WebRtc_UWord32      _packetRate;
        WebRtc_UWord8       _bitrateNextIdx;
        WebRtc_UWord32      _packetRateArray[10];
        WebRtc_UWord32      _bitrateArray[10];
        WebRtc_UWord32      _bitrateDiffMS[10];
        WebRtc_UWord32      _timeLastRateUpdate;
        WebRtc_UWord32      _bytesCount;
        WebRtc_UWord32      _packetCount;

        OverUseDetector _overUseDector;
};

} //end namespace BWE
#endif
