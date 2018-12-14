#if _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include "BWE_rate_control.h"
#include <math.h>
#include <stdlib.h> //fabsf
#include <assert.h>

#define INIT_CAPACITY_SLOPE 8.0/512.0
#define DETECTOR_THRESHOLD 30.0  //change from 25 to 60
#define OVER_USING_TIME_THRESHOLD 100
#define MIN_FRAME_PERIOD_HISTORY_LEN 4 //change from 60 to 4, when frame rate is low, 60 is just too large

namespace bwe {
OverUseDetector::OverUseDetector()
:
_firstPacket(true),
_currentFrame(),
_prevFrame(),
_numOfDeltas(0),
_slope(INIT_CAPACITY_SLOPE),
_offset(0),
_E(),
_processNoise(),
_avgNoise(0.0),
_varNoise(500),
_threshold(DETECTOR_THRESHOLD),
_tsDeltaHist(),
_prevOffset(0.0),
_timeOverUsing(-1),
_overUseCounter(0),
_hypothesis(kBwNormal),
_lastSeqNo(0),
_lastUpdateTimeMS(0),
_tTsDeltaJitter(15.0),
_posiFrameDelta(100.0),
_posiTsDelta(1.0)
{
    _E[0][0] = 100;
    _E[1][1] = 1e-1;
    _E[0][1] = _E[1][0] = 0;
    _processNoise[0] = 1e-10;
    _processNoise[1] = 1e-2;
}

OverUseDetector::~OverUseDetector()
{
    _tsDeltaHist.clear();
}

void OverUseDetector::Reset()
{
    _firstPacket = true;
    _currentFrame._size = 0;
    _currentFrame._completeTimeMs = -1;
    _currentFrame._timestamp = -1;
    _prevFrame._size = 0;
    _prevFrame._completeTimeMs = -1;
    _prevFrame._timestamp = -1;
    _numOfDeltas = 0;
    _slope = INIT_CAPACITY_SLOPE;
    _offset = 0;
    _E[0][0] = 100;
    _E[1][1] = 1e-1;
    _E[0][1] = _E[1][0] = 0;
    _processNoise[0] = 1e-10;
    _processNoise[1] = 1e-2;
    _avgNoise = 0.0;
    _varNoise = 500;
    _threshold = DETECTOR_THRESHOLD;
    _prevOffset = 0.0;
    _timeOverUsing = -1;
    _overUseCounter = 0;
    _hypothesis = kBwNormal;
    _tsDeltaHist.clear();
    _lastSeqNo = 0;
    _lastUpdateTimeMS = 0;
    _tTsDeltaJitter = 15.0;
    _posiFrameDelta = 100.0;
    _posiTsDelta = 1.0;
}

bool OverUseDetector::OldSequence(WebRtc_UWord16 newSeq, WebRtc_UWord32 existingSeq)
{
    bool tmpWrapped =
        (newSeq < 0x00ff && existingSeq > 0xff00) ||
        (newSeq > 0xff00 && existingSeq < 0x00ff);

    if (existingSeq > newSeq && !tmpWrapped) {
        return true;
    } else if (existingSeq <= newSeq && !tmpWrapped) {
        return false;
    } else if (existingSeq < newSeq && tmpWrapped) {
        return true;
    } else {
        return false;
    }    
}

bool OverUseDetector::OldTimestamp(uint32_t newTimestamp, uint32_t existingTimestamp, bool* wrapped)
{
         bool tmpWrapped =
                   (newTimestamp < 0x0000ffff && existingTimestamp > 0xffff0000) ||
                    (newTimestamp > 0xffff0000 && existingTimestamp < 0x0000ffff);
         *wrapped = tmpWrapped;
         if (existingTimestamp > newTimestamp && !tmpWrapped) {
                return true;
         } else if (existingTimestamp <= newTimestamp && !tmpWrapped) {
                return false;
         } else if (existingTimestamp < newTimestamp && tmpWrapped) {
                return true;
         } else {
                return false;
         }
}

bool OverUseDetector::Update(const WebRtc_UWord64 timestamp, const WebRtc_UWord64 sequenceNumber,
                             const WebRtc_UWord16 packetSize,
                             const WebRtc_Word64 nowMS)
{
    bool wrapped = false;
    bool completeFrame = false;
    if (_currentFrame._timestamp == -1)
    {
        _currentFrame._timestamp = timestamp;
        _lastSeqNo = sequenceNumber;
    }
    else if (OldSequence(sequenceNumber,_lastSeqNo) ||
             OldTimestamp(timestamp, static_cast<WebRtc_UWord32>(_currentFrame._timestamp),
                 &wrapped))
    {
        // Don't update with old data
        return completeFrame;
    }
    else
    {
        _currentFrame._timestamp = timestamp;
        _currentFrame._size = packetSize;
        _currentFrame._completeTimeMs = nowMS;
        // First packet of a later frame, the previous frame sample is ready
        if (_prevFrame._completeTimeMs >= 0) // This is our second frame
        {
            WebRtc_Word64 tDelta = 0;
            double tsDelta = 0;
            // Check for wrap
            OldTimestamp(
                static_cast<WebRtc_UWord32>(_prevFrame._timestamp),
                static_cast<WebRtc_UWord32>(_currentFrame._timestamp),
                &wrapped);
            CompensatedTimeDelta(_currentFrame, _prevFrame, tDelta, tsDelta, wrapped);
            double tTsDelta = tDelta - tsDelta;
            double fsDelta = static_cast<double>(_currentFrame._size) - _prevFrame._size;

//#define WEBRTC_KALMAN_OVERUSE_DETECT
#ifdef WEBRTC_KALMAN_OVERUSE_DETECT
            //WebRTC implement the senior complex version of overuse detector
            // It's more sensitive while do not stable enough for some cases like time drift
            UpdateKalman(tDelta, tsDelta, _currentFrame._size, _prevFrame._size);
#else
            // We re-use the overuse detector architecture, and develope a simple method
            // to detect the overuse event.
            SimpleDetect(tTsDelta,  fsDelta, nowMS);
#endif
        }
        // The new timestamp is now the current frame,
        // and the old timestamp becomes the previous frame.
        _prevFrame._completeTimeMs = _currentFrame._completeTimeMs;
        _prevFrame._size= _currentFrame._size;
        _prevFrame._timestamp= _currentFrame._timestamp;
        _lastSeqNo = sequenceNumber;
        
        completeFrame = true;
    }

    return completeFrame;
}

BandwidthUsage OverUseDetector::State() const
{
    return _hypothesis;
}

double OverUseDetector::NoiseVar() const
{
    return _varNoise;
}

void OverUseDetector::SetRateControlRegion(RateControlRegion region)
{
    switch (region)
    {
    case kRcMaxUnknown:
        {
            _threshold = DETECTOR_THRESHOLD;
            break;
        }
    case kRcAboveMax:
    case kRcNearMax:
        {
            _threshold = DETECTOR_THRESHOLD / 2;
            break;
        }
    }
}

void OverUseDetector::CompensatedTimeDelta(const FrameSample& currentFrame, const FrameSample& prevFrame, WebRtc_Word64& tDelta,
                                           double& tsDelta, bool wrapped)
{
    _numOfDeltas++;
    if (_numOfDeltas > 1000)
    {
        _numOfDeltas = 1000;
    }
    // Add wrap-around compensation
    WebRtc_Word64 wrapCompensation = 0;
    if (wrapped)
    {
        wrapCompensation = static_cast<WebRtc_Word64>(1)<<32;
    }
    tsDelta = (currentFrame._timestamp + wrapCompensation - prevFrame._timestamp) / 90.0;
    tDelta = currentFrame._completeTimeMs - prevFrame._completeTimeMs;
    //assert(tsDelta > 0);
}

double OverUseDetector::CurrentDrift()
{
    return 1.0;
}

void OverUseDetector::UpdateKalman(WebRtc_Word64 tDelta, double tsDelta, 
                WebRtc_UWord32 frameSize, WebRtc_UWord32 prevFrameSize)
{
    const double minFramePeriod = UpdateMinFramePeriod(tsDelta);
    const double drift = CurrentDrift();
    // Compensate for drift
    const double tTsDelta = tDelta - tsDelta / drift;
    double fsDelta = static_cast<double>(frameSize) - prevFrameSize;

    // Update the Kalman filter
    const double scaleFactor =  minFramePeriod / (1000.0 / 30.0);
    _E[0][0] += _processNoise[0] * scaleFactor;
    _E[1][1] += _processNoise[1] * scaleFactor;

    if ((_hypothesis == kBwOverusing && _offset < _prevOffset) ||
        (_hypothesis == kBwUnderUsing && _offset > _prevOffset))
    {
        _E[1][1] += 10 * _processNoise[1] * scaleFactor;
    }

    const double h[2] = {fsDelta, 1.0};
    const double Eh[2] = {_E[0][0]*h[0] + _E[0][1]*h[1],
                         _E[1][0]*h[0] + _E[1][1]*h[1]};

    const double residual = tTsDelta - _slope*h[0] - _offset;

    const bool stableState = (BWE_MIN(_numOfDeltas, 60) * fabs(_offset) < _threshold);
    // We try to filter out very late frames. For instance periodic key
    // frames doesn't fit the Gaussian model well.
    /*if (fabsf(residual) < 3 * sqrt(_varNoise))
    {
        UpdateNoiseEstimate(residual, minFramePeriod, stableState);
    }
    else
    {
        UpdateNoiseEstimate(3 * sqrt(_varNoise), minFramePeriod, stableState);
    }*/
    //Vik:We should let the kalman deal this automatically, 
    //or the jitter will be as offset and there's many under-use and over-use event
    UpdateNoiseEstimate(residual, minFramePeriod, stableState);

    const double denom = _varNoise + h[0]*Eh[0] + h[1]*Eh[1];

    const double K[2] = {Eh[0] / denom,
                        Eh[1] / denom};

    const double IKh[2][2] = {{1.0 - K[0]*h[0], -K[0]*h[1]},
                             {-K[1]*h[0], 1.0 - K[1]*h[1]}};
    const double e00 = _E[0][0];
    const double e01 = _E[0][1];

    // Update state
    _E[0][0] = e00 * IKh[0][0] + _E[1][0] * IKh[0][1];
    _E[0][1] = e01 * IKh[0][0] + _E[1][1] * IKh[0][1];
    _E[1][0] = e00 * IKh[1][0] + _E[1][0] * IKh[1][1];
    _E[1][1] = e01 * IKh[1][0] + _E[1][1] * IKh[1][1];

    // Covariance matrix, must be positive semi-definite
    assert(_E[0][0] + _E[1][1] >= 0 &&
           _E[0][0] * _E[1][1] - _E[0][1] * _E[1][0] >= 0 &&
           _E[0][0] >= 0);

    _slope = _slope + K[0] * residual;
    _slope = BWE_MAX(_slope, 1e-7f); //make sure _slope not be zero
    _prevOffset = _offset;
    _offset = _offset + K[1] * residual;

    Detect(tsDelta);

    double instaCapacityKbps = 0;
    instaCapacityKbps = frameSize * 8 / (tDelta + 0.001);

}

double OverUseDetector::UpdateMinFramePeriod(double tsDelta) {
  double minFramePeriod = tsDelta;
  if (_tsDeltaHist.size() >= MIN_FRAME_PERIOD_HISTORY_LEN) {
    std::list<double>::iterator firstItem = _tsDeltaHist.begin();
    _tsDeltaHist.erase(firstItem);
  }
  std::list<double>::iterator it = _tsDeltaHist.begin();
  for (; it != _tsDeltaHist.end(); it++) {
    minFramePeriod = BWE_MIN(*it, minFramePeriod);
  }
  _tsDeltaHist.push_back(tsDelta);
  return minFramePeriod;
}

void OverUseDetector::UpdateNoiseEstimate(double residual, double tsDelta, bool stableState)
{
    if (!stableState)
    {
        return;
    }
    // Faster filter during startup to faster adapt to the jitter level of the network
    // alpha is tuned for 30 frames per second, but
    double alpha = 0.01;
    if (_numOfDeltas > 10*30)
    {
        alpha = 0.002;
    }
    // Only update the noise estimate if we're not over-using
    // beta is a function of alpha and the time delta since
    // the previous update.
    const double beta = pow(1 - alpha, tsDelta * 30.0 / 1000.0);
    _avgNoise = beta * _avgNoise + (1 - beta) * residual;
    _varNoise = beta * _varNoise + (1 - beta) * (_avgNoise - residual) * (_avgNoise - residual);
    if (_varNoise < 1e-7)
    {
        _varNoise = 1e-7;
    }
}

void OverUseDetector::UpdateKalmanEx(int64_t t_delta,
                              double ts_delta,
                              int size_delta,
                              BandwidthUsage current_hypothesis) {
  const double min_frame_period = UpdateMinFramePeriodEx(ts_delta);
  const double t_ts_delta = t_delta - ts_delta;
  double fs_delta = size_delta;

  ++_numOfDeltas;
  if (_numOfDeltas > kDeltaCounterMax) {
    _numOfDeltas = kDeltaCounterMax;
  }

  // Update the Kalman filter.
  _E[0][0] += _processNoise[0];
  _E[1][1] += _processNoise[1];

  if ((current_hypothesis == kBwOverusing && _offset < _prevOffset) ||
      (current_hypothesis == kBwUnderUsing && _offset > _prevOffset)) {
    _E[1][1] += 10 * _processNoise[1];
  }

  const double h[2] = {fs_delta, 1.0};
  const double Eh[2] = {_E[0][0]*h[0] + _E[0][1]*h[1],
                        _E[1][0]*h[0] + _E[1][1]*h[1]};

  const double residual = t_ts_delta - _slope*h[0] - _offset;

  const bool in_stable_state = (current_hypothesis == kBwNormal);
  const double max_residual = 3.0 * sqrt(_varNoise);
  // We try to filter out very late frames. For instance periodic key
  // frames doesn't fit the Gaussian model well.
  if (fabs(residual) < max_residual) {
    UpdateNoiseEstimateEx(residual, min_frame_period, in_stable_state);
  } else {
    UpdateNoiseEstimateEx(residual < 0 ? -max_residual : max_residual,
                        min_frame_period, in_stable_state);
  }

  const double denom = _varNoise + h[0]*Eh[0] + h[1]*Eh[1];

  const double K[2] = {Eh[0] / denom,
                       Eh[1] / denom};

  const double IKh[2][2] = {{1.0 - K[0]*h[0], -K[0]*h[1]},
                            {-K[1]*h[0], 1.0 - K[1]*h[1]}};
  const double e00 = _E[0][0];
  const double e01 = _E[0][1];

  // Update state.
  _E[0][0] = e00 * IKh[0][0] + _E[1][0] * IKh[0][1];
  _E[0][1] = e01 * IKh[0][0] + _E[1][1] * IKh[0][1];
  _E[1][0] = e00 * IKh[1][0] + _E[1][0] * IKh[1][1];
  _E[1][1] = e01 * IKh[1][0] + _E[1][1] * IKh[1][1];

  // The covariance matrix must be positive semi-definite.
  bool positive_semi_definite = _E[0][0] + _E[1][1] >= 0 &&
      _E[0][0] * _E[1][1] - _E[0][1] * _E[1][0] >= 0 && _E[0][0] >= 0;
  assert(positive_semi_definite);

  _slope = _slope + K[0] * residual;
  _prevOffset = _offset;
  _offset = _offset + K[1] * residual;
  DetectEx(ts_delta);
}

double OverUseDetector::UpdateMinFramePeriodEx(double ts_delta) {
  double min_frame_period = ts_delta;
  if (_tsDeltaHist.size() >= kMinFramePeriodHistoryLength) {
    _tsDeltaHist.pop_front();
  }
  std::list<double>::iterator it = _tsDeltaHist.begin();
  for (; it != _tsDeltaHist.end(); it++) {
    min_frame_period = std::min(*it, min_frame_period);
  }
  _tsDeltaHist.push_back(ts_delta);
  return min_frame_period;
}

void OverUseDetector::UpdateNoiseEstimateEx(double residual,
                                           double ts_delta,
                                           bool stable_state) {
  if (!stable_state) {
    return;
  }
  // Faster filter during startup to faster adapt to the jitter level
  // of the network. |alpha| is tuned for 30 frames per second, but is scaled
  // according to |ts_delta|.
  double alpha = 0.01;
  if (_numOfDeltas > 10*30) {
    alpha = 0.002;
  }
  // Only update the noise estimate if we're not over-using. |beta| is a
  // function of alpha and the time delta since the previous update.
  const double beta = pow(1 - alpha, ts_delta * 30.0 / 1000.0);
  _avgNoise = beta * _avgNoise
              + (1 - beta) * residual;
  _varNoise = beta * _varNoise
              + (1 - beta) * (_avgNoise - residual) * (_avgNoise - residual);
  if (_varNoise < 1) {
    _varNoise = 1;
  }
}

BandwidthUsage OverUseDetector::DetectEx(double ts_delta) {
  if (_numOfDeltas < 2) {
    return kBwNormal;
  }
  const double prev_offset = _prevOffset;
  _prevOffset = _offset;
  const double T = BWE_MIN(_numOfDeltas, 60) * _offset;

  if (T > _threshold) {
    if (_timeOverUsing == -1) {
      // Initialize the timer. Assume that we've been
      // over-using half of the time since the previous
      // sample.
      _timeOverUsing = ts_delta / 2;
    } else {
      // Increment timer
      _timeOverUsing += ts_delta;
    }
    _overUseCounter++;
    if (_timeOverUsing > OVER_USING_TIME_THRESHOLD && _overUseCounter > 1) {
      if (_offset >= prev_offset) {
        _timeOverUsing = 0;
        _overUseCounter = 0;
        _hypothesis = kBwOverusing;
      }
    }
  } else if (T < -_threshold) {
    _timeOverUsing = -1;
    _overUseCounter = 0;
    _hypothesis = kBwUnderUsing;
  } else {
    _timeOverUsing = -1;
    _overUseCounter = 0;
    _hypothesis = kBwNormal;
  }

  return _hypothesis;
}

BandwidthUsage OverUseDetector::Detect(double tsDelta)
{
    if (_numOfDeltas < 2)
    {
        return kBwNormal;
    }
    const double T = BWE_MIN(_numOfDeltas, 60) * _offset;
    if (T > _threshold)
    {
        if (_timeOverUsing == -1)
        {
            // Initialize the timer. Assume that we've been
            // over-using half of the time since the previous
            // sample.
            _timeOverUsing = tsDelta / 2;
        }
        else
        {
            // Increment timer
            _timeOverUsing += tsDelta;
        }
        _overUseCounter++;
        if (_timeOverUsing > OVER_USING_TIME_THRESHOLD && _overUseCounter > 1)
        {
            if (_offset >= _prevOffset)
            {   
                _timeOverUsing = 0;
                _overUseCounter = 0;
                _hypothesis = kBwOverusing;
            }
        }
    }
    else if (T < -DETECTOR_THRESHOLD)
    {
        _timeOverUsing = -1;
        _overUseCounter = 0;
        _hypothesis = kBwUnderUsing;
    }
    else
    {
        _timeOverUsing = -1;
        _overUseCounter = 0;
        _hypothesis = kBwNormal;
    }
    return _hypothesis;
}

void OverUseDetector::SimpleDetect(double tTsDelta, double frameSizeDelta, 
                const WebRtc_Word64 nowMS)
{
    double T;
    double bwCapacity;
    double sizeDelay = 0;
    double tSizeDelta = 0;

    tTsDelta = BWE_MAX(tTsDelta, -10000);
    tTsDelta = BWE_MIN(tTsDelta, 10000);
    
    _tTsDeltaJitter = 0.99 * _tTsDeltaJitter + 0.01 * fabs(tTsDelta);

    if (tTsDelta > 0 && frameSizeDelta > 0)
    {
        _posiTsDelta = 0.99 * _posiTsDelta + 0.01 * tTsDelta;
        _posiFrameDelta = 0.99 * _posiFrameDelta + 0.01 * frameSizeDelta;

        tSizeDelta = _posiTsDelta - _tTsDeltaJitter;
        tSizeDelta = BWE_MAX(tSizeDelta, 0);
        
        bwCapacity = _posiFrameDelta/ (tSizeDelta + 0.001);
        //To avoid big key frame size detected as be overuse
        sizeDelay = frameSizeDelta/(bwCapacity+0.001);
        sizeDelay = BWE_MAX(sizeDelay, 0);
        sizeDelay = BWE_MIN(sizeDelay, 150); //we do not allow a key frame delay more than 150ms
    }

    double step = 0.008;
    int multiple = 1;
    if (_lastUpdateTimeMS > 0)
    {
        multiple = (nowMS - _lastUpdateTimeMS + 15)/30;
        multiple = BWE_MAX(multiple, 1);
        multiple = BWE_MIN(multiple, 30);
    }    
    _offset = _offset / (1 + 0.008 * multiple) + tTsDelta;


    _lastUpdateTimeMS = nowMS;

    // the mean of Half-normal distribution is around 0.79
    // and we assume to let the normalized CDF be 2.5 (cover the 0.997 probability)
    // 2.5/0.79 = 3.16
    // therefor we set the threshold to offset is mean jitter (_tTsDeltaJitter) * 3.16
    T = _offset - _tTsDeltaJitter * 3.16 - sizeDelay;
    if (T > 170.0)
    {
        _hypothesis = kBwOverusing;
    }
    else if (T <= 170.0)
    {
        _hypothesis = kBwNormal;
    }
    else
    {
        _hypothesis = kBwUnderUsing;

    }  
}

RemoteRateControl::RemoteRateControl()
:
_minConfiguredBitRate(10000),
_maxConfiguredBitRate(30000000),
_currentBitRate(_maxConfiguredBitRate),
_maxHoldRate(0),
_avgMaxBitRateKbps(-1.0f),
_varMaxBitRateKbps(0.4f),
_rcState(kRcIncrease),
_cameFromState(kRcDecrease),
_rcRegion(kRcMaxUnknown),
_lastBitRateChange(-1),
_updated(false),
_lastChangeMs(-1),
_lastChangeBitrate(0),
_initializedBitRate(false),
_timeFirstIncomingEstimate(-1),
_beta(0.9f),
_overUseRate(0),
_lloverUseRate(0.002), //we consider not too sanguine
_meanOverUseBr(0),
_avgRTTFar(0),
_avgRTTNear(0),
_rttThreshold(2000),
_avgLoss(0),
_varLoss(100),
_prevAccumLoss(0),
_prevMaxSeqNo(0),
_prevTime(0),
_numUpdate(0),
_overusePacketCount(0),
_initIncomingRate(0),
_enableLossMetric(true),
_lossHypothesis(kBwNormal),
_baseLoss(0),
_normalCount(0),
_lastTimeChangeRate(0),
_bitratePercent(0),
_minCpuLimitedBr(0),
_timeLastBwReport(0),
_initialState(true),
_overUseDector(),
_state(kNetworkBwUnknown),
_firstOverUse(true),
_asufficBitRate(684000), //default bitrate is 684kbps, it's enough for 20hz VGA
_packetRate(0),
_bitrateNextIdx(0),
_timeLastRateUpdate(0),
_bytesCount(0),
_packetCount(0),
_currentInput(kBwNormal, 0, 1.0)
{
    memset(_packetRateArray, 0, sizeof(_packetRateArray));
    memset(_bitrateDiffMS, 0, sizeof(_bitrateDiffMS));
    memset(_bitrateArray, 0, sizeof(_bitrateArray));
}

RemoteRateControl::~RemoteRateControl()
{
}

void RemoteRateControl::Reset()
{
    _minConfiguredBitRate = 10000;
    _maxConfiguredBitRate = 30000000;
    _currentBitRate = _maxConfiguredBitRate;
    _maxHoldRate = 0;
    _avgMaxBitRateKbps = -1.0f;
    _varMaxBitRateKbps = 0.4f;
    _rcState = kRcIncrease;
    _cameFromState = kRcHold;
    _rcRegion = kRcMaxUnknown;
    _lastBitRateChange = -1;
    _lastChangeMs = -1;
    _lastChangeBitrate = 0;
    _beta = 0.9f;
    _currentInput._bwState = kBwNormal;
    _currentInput._incomingBitRate = 0;
    _currentInput._noiseVar = 1.0;
    _updated = false;
    _initializedBitRate = false;
    _timeFirstIncomingEstimate = -1;
    _overUseRate = 0;
    _lloverUseRate = 0.002;
    _meanOverUseBr = 0;
    _avgRTTFar = 0;
    _avgRTTNear = 0;
    _rttThreshold = 2000;

    _initIncomingRate = 0;

    _avgLoss = 0;
    _varLoss = 100;
    _lossHypothesis = kBwNormal;
    _prevAccumLoss = 0;
    _prevMaxSeqNo = 0;
    _prevTime = 0;
    _numUpdate = 0;
    _overusePacketCount = 0;
    _enableLossMetric = true;
    _baseLoss = 0;
    _normalCount = 0;
    _lastTimeChangeRate = 0;
    _bitratePercent = 0;
    _minCpuLimitedBr = 0;
    _timeLastBwReport = 0;
    _overUseDector.Reset();
    //_asufficBitRate = 684000; don't reset this one since it will be informed from remote side via rtcp
}

bool RemoteRateControl::ValidEstimate() const {
    return _initializedBitRate;
}

WebRtc_Word32 RemoteRateControl::SetConfiguredBitRates(WebRtc_UWord32 minBitRateBps, WebRtc_UWord32 maxBitRateBps)
{
    if (minBitRateBps > maxBitRateBps)
    {
        return -1;
    }
    _minConfiguredBitRate = minBitRateBps;
    _maxConfiguredBitRate = maxBitRateBps;
    _currentBitRate = BWE_MIN(BWE_MAX(minBitRateBps, _currentBitRate), maxBitRateBps);
    return 0;
}

WebRtc_UWord32 RemoteRateControl::LatestEstimate() const {
    return _currentBitRate;
}

NetwBandwidthStatus RemoteRateControl::GetBandWidthStatus() const {
    return _state;
}

WebRtc_UWord32 RemoteRateControl::UpdateBandwidthEstimate(WebRtc_UWord32 RTT,
                                                          WebRtc_Word64 nowMS, float remoteTargetBrRatio, bool rtcpTimeout /*= false*/)
{
    _currentBitRate = ChangeBitRate(_currentBitRate, _currentInput._incomingBitRate,
        _currentInput._noiseVar, RTT, nowMS, remoteTargetBrRatio);

    if (_enableLossMetric && _lossHypothesis == kBwOverusing && _rcState != kRcDecrease
        && RTT > _rttThreshold + _avgRTTFar)
    {  //if detect overuse is failed,
        //use loss metric and rtt to detect overuse
        float beta = 0.9f;
        if (_lastTimeChangeRate > 0)
        {
            beta = pow(0.9f,(float)(nowMS-_lastTimeChangeRate)/1000);
        }
        beta = BWE_MIN(BWE_MAX(beta,0.80f),0.99f);
        _lastTimeChangeRate = nowMS;
        _currentBitRate = BWE_MIN(_currentBitRate,_currentInput._incomingBitRate*beta);
    }
    else
    {
        _lastTimeChangeRate = 0;
    }

    if (_bitratePercent > 0 && _rcState != kRcDecrease)
    {//limit  _currentBitRate by _bitratePercent if it is set
        _currentBitRate *= _bitratePercent;
        _bitratePercent = 0;
        if (_currentBitRate < _minCpuLimitedBr)
        {
            _currentBitRate = _minCpuLimitedBr;
        }
        _minCpuLimitedBr = 0;
    }

    if (_timeLastBwReport == 0 || nowMS - _timeLastBwReport > kBwReportInterval)
    {
        NetwBandwidthStatus state;
        _timeLastBwReport = nowMS;
        if (_initialState)
        {
            //assume at least normal state in init phase
            if (_currentBitRate > kBwThGood)
                state = kNetworkBwVeryGood;
            else if (_currentBitRate >= kBwThNormal)
                state = kNetworkBwGood;
            else
                state = kNetworkBwNormal;
        }
        else
        {
            if (_currentBitRate <= kBwThVeryLow)
                state = kNetworkBwVeryLow;
            else if (_currentBitRate <= kBwThLow)
                state = kNetworkBwLow;
            else if (_currentBitRate <= kBwThNormal)
                state = kNetworkBwNormal;
            else if (_currentBitRate <= kBwThGood)
                state = kNetworkBwGood;
            else
                state = kNetworkBwVeryGood;
        }
        _state = state;
    }

    _currentBitRate = BWE_MIN(BWE_MAX(_minConfiguredBitRate, _currentBitRate), _maxConfiguredBitRate);
    return _currentBitRate;
}

RateControlRegion RemoteRateControl::Update(const RateControlInput& input,
                                            bool& firstOverUse,
                                            WebRtc_Word64 nowMS,
                                            WebRtc_UWord32 tmmbnRatekbit)
{
    firstOverUse = (_currentInput._bwState != kBwOverusing &&
        input._bwState == kBwOverusing);

    // adopt rate estimation after one second since we are receiving
    if (!_initializedBitRate)
    {
        _initIncomingRate = BWE_MAX(_initIncomingRate,input._incomingBitRate);
        if (input._bwState == kBwOverusing)
        {
            if (_timeFirstIncomingEstimate < 0)
                _timeFirstIncomingEstimate = nowMS;
            _currentBitRate = _initIncomingRate;
            _initializedBitRate = true;
        }
        else if (_timeFirstIncomingEstimate < 0)
        {
            if (input._incomingBitRate > 0)
            {
                _timeFirstIncomingEstimate = nowMS;
            }
        }
        else if (tmmbnRatekbit>0 && _initIncomingRate >= tmmbnRatekbit*1000*0.5)
        {//initialize bitrate by tmmbn
            _currentBitRate = BWE_MIN(tmmbnRatekbit*1000,_initIncomingRate);
            _initializedBitRate = true;
        }
        else if (nowMS - _timeFirstIncomingEstimate > 2000 &&
            _initIncomingRate > 0)
        {//initialize bitrate by max incoming bitrate if we have not receive a valid TMMBN from sender in 3s
            _currentBitRate = _initIncomingRate;
            _initializedBitRate = true;
        }
    }

    if (_updated && _currentInput._bwState == kBwOverusing)
    {
        // Only update delay factor and incoming bit rate. We always want to react on an over-use.
        _currentInput._noiseVar = input._noiseVar;
        _currentInput._incomingBitRate = input._incomingBitRate;
        return _rcRegion;
    }
    _updated = true;
    _currentInput = input;
    return _rcRegion;
}


void RemoteRateControl::UpdatePacket(const WebRtc_UWord64 timestamp, const WebRtc_UWord64 sequenceNumber,
                                   const WebRtc_UWord16 packetSize, const WebRtc_Word64 nowMS)
{
    _overUseDector.Update(timestamp, sequenceNumber, packetSize, nowMS);
    WebRtc_Word64 diffMS = nowMS - _lastChangeMs;
    if(_lastChangeMs == -1) {
        _lastChangeMs = nowMS;
        return;
    }
    _bytesCount += packetSize;
    _packetCount++;
    ProcessBitRate(nowMS);
    WebRtc_UWord32 incomingBitRate = (((WebRtc_UWord64)_lastChangeBitrate * 1000) + packetSize)/(1000+diffMS);
    double noiseVar = 0;
    BandwidthUsage  bwState = _overUseDector.State();
    UpdateDetection(incomingBitRate, noiseVar, bwState);
    _overUseDector.SetRateControlRegion(_rcRegion);
    _lastChangeMs = nowMS;
    _updated = true;
}

void RemoteRateControl::ProcessBitRate(const WebRtc_Word64 nowMS)
{
    // triggered by timer
    WebRtc_UWord32 diffMS = nowMS -_timeLastRateUpdate;

    if(diffMS > 100)
    {
        if(diffMS > 10000) // 10 sec
        {
            // too high diff ignore
            _timeLastRateUpdate = nowMS;
            _bytesCount = 0;
            _packetCount = 0;
            return;
        }
        _packetRateArray[_bitrateNextIdx] = (_packetCount*1000)/diffMS;
        _bitrateArray[_bitrateNextIdx]    = 8*((_bytesCount*1000)/diffMS);
        // will overflow at ~34 Mbit/s
        _bitrateDiffMS[_bitrateNextIdx]   = diffMS;
        _bitrateNextIdx++;
        if(_bitrateNextIdx >= 10)
        {
            _bitrateNextIdx = 0;
        }

        WebRtc_UWord32 sumDiffMS = 0;
        WebRtc_UWord64 sumBitrateMS = 0;
        WebRtc_UWord32 sumPacketrateMS = 0;
        for(int i= 0; i <10; i++)
        {
            // sum of time
            sumDiffMS += _bitrateDiffMS[i];
            sumBitrateMS += _bitrateArray[i] * _bitrateDiffMS[i];
            sumPacketrateMS += _packetRateArray[i] * _bitrateDiffMS[i];
        }
        _timeLastRateUpdate = nowMS;
        _bytesCount = 0;
        _packetCount = 0;

        _packetRate = sumPacketrateMS/sumDiffMS;
        _lastChangeBitrate = WebRtc_UWord32(sumBitrateMS / sumDiffMS);
    }
}


void RemoteRateControl::UpdateDetection(WebRtc_UWord32 incomingBitRate, double noiseVar, BandwidthUsage  bwState)
{
    if (_updated && _currentInput._bwState == kBwOverusing)
    {
        _currentInput._noiseVar = noiseVar;
        _currentInput._incomingBitRate = incomingBitRate;
        return;
    }
    _currentInput._noiseVar = noiseVar;
    _currentInput._incomingBitRate = incomingBitRate;
    _currentInput._bwState = bwState;
}

WebRtc_UWord32 RemoteRateControl::ChangeBitRate(WebRtc_UWord32 currentBitRate,
                                                WebRtc_UWord32 incomingBitRate,
                                                double noiseVar,
                                                WebRtc_UWord32 RTT,
                                                WebRtc_Word64 nowMS,
                                                float remoteTargetBrRatio)
{
    if (!_updated)
    {
        return _currentBitRate;
    }
    _updated = false;

    if (_enableLossMetric)
    {//compensate for loss
        incomingBitRate = incomingBitRate*(1+BWE_MIN(_avgLoss,128.0f)/255);
    }

    UpdateRTT(RTT);
    ChangeState(_currentInput, nowMS);
    UpdateOveruseRate(incomingBitRate, _rcState);

    const float incomingBitRateKbps = incomingBitRate / 1000.0f;
    // Calculate the max bit rate std dev given the normalized
    // variance and the current incoming bit rate.
    const float stdMaxBitRate = sqrt(_varMaxBitRateKbps * _avgMaxBitRateKbps);
    const float origBitRate = static_cast<float>(currentBitRate);

    WebRtc_UWord32 deltaInc = 0;
    WebRtc_UWord32 reactionTimeMs = 0;

    if (-1 == _lastBitRateChange)
    {
        reactionTimeMs = 1000;
    }
    else
    {
        reactionTimeMs = static_cast<WebRtc_UWord32>(nowMS - _lastBitRateChange + 0.5f);
        reactionTimeMs = BWE_MIN(3000, reactionTimeMs); //Sanity
        reactionTimeMs = BWE_MAX(20, reactionTimeMs);
    }

    switch (_rcState)
    {
    case kRcHold:
        {
            _firstOverUse = true;
            _maxHoldRate = BWE_MAX(_maxHoldRate, incomingBitRate);

            deltaInc = 300 * reactionTimeMs /1000;
            currentBitRate = currentBitRate + deltaInc;

            break;
        }
    case kRcIncrease:
        {
            _firstOverUse = true;
            if (_avgMaxBitRateKbps >= 0)
            {
                if (incomingBitRateKbps > _avgMaxBitRateKbps + 3 * stdMaxBitRate)
                {
                    ChangeRegion(kRcMaxUnknown);
                    _avgMaxBitRateKbps = -1.0;
                }
                else if (incomingBitRateKbps > _avgMaxBitRateKbps + 2.5 * stdMaxBitRate)
                {
                    ChangeRegion(kRcAboveMax);
                }
            }
            float incommingRatio = incomingBitRate / (currentBitRate + 0.1f);

            double alpha = CalcRateIncrease(currentBitRate, reactionTimeMs, incommingRatio);
            //double alpha = DeltaRateIncrease(reactionTimeMs);

            deltaInc = 3000 * reactionTimeMs /1000;

            currentBitRate = static_cast<WebRtc_UWord32>(currentBitRate * alpha) + deltaInc;

            if (_maxHoldRate > 0 && _beta * _maxHoldRate > currentBitRate)
            {
                currentBitRate = static_cast<WebRtc_UWord32>(_beta * _maxHoldRate);
                _avgMaxBitRateKbps = currentBitRate / 1000.0f;
                ChangeRegion(kRcNearMax);
            }
            _maxHoldRate = 0;

            break;
        }
    case kRcDecrease:
        {
            if (_initialState)
                _initialState = false;
            _maxHoldRate = BWE_MIN(_maxHoldRate, incomingBitRate);

            if (incomingBitRate < _minConfiguredBitRate)
            {
                currentBitRate = _minConfiguredBitRate - 1000;
            }
            else
            {

                _beta = BWE_MIN(1.0, _beta);
                float beta = 0.9f;
                if(_firstOverUse == false)
                   beta = pow(_beta, reactionTimeMs/800.0f);
                _firstOverUse = false;
                //beta = BWE_MAX(_beta, beta);

                if(remoteTargetBrRatio > 0.0f)
                    incomingBitRate = incomingBitRate/remoteTargetBrRatio;

                // to prevent it from reducing too much.
                if(currentBitRate < incomingBitRate * 2)
                    currentBitRate = static_cast<WebRtc_UWord32>(beta * incomingBitRate - 5000);
                else
                    currentBitRate = incomingBitRate;

                if(currentBitRate > origBitRate)
                    currentBitRate = origBitRate;

                if (currentBitRate > _currentBitRate)
                {
                    // Avoid increasing the rate when over-using.
                    //if (_rcRegion != kRcMaxUnknown)
                    //{
                    //    currentBitRate = static_cast<WebRtc_UWord32>(_beta * _avgMaxBitRateKbps * 1000 + 0.5f);
                    //}
                    currentBitRate = BWE_MIN(currentBitRate, _currentBitRate);
                }
                currentBitRate = BWE_MAX(currentBitRate, _minConfiguredBitRate);
                //ChangeRegion(kRcNearMax);

                if (incomingBitRateKbps < _avgMaxBitRateKbps - 3 * stdMaxBitRate)
                {
                    _avgMaxBitRateKbps = -1.0f;
                }

                UpdateMaxBitRateEstimate(incomingBitRateKbps);
            }
            break;
        }
    }

    if (currentBitRate > 1.5 * incomingBitRate
        && currentBitRate > _asufficBitRate/3
        && currentBitRate > 100000)
    {
        // Don't change the bit rate if the send side is too far off
        currentBitRate = _currentBitRate;
    }

    _lastBitRateChange = nowMS;
    return currentBitRate;
}

double
RemoteRateControl::CalcRateIncrease(WebRtc_UWord32 currentBitRate,
                                    WebRtc_UWord32 reactionTimeMs, float incommingRatio) const
{
    const double cTa2 = -0.06;
    const double cTa3 = 0.021;

    double alpha = 1.05f;// 1.015f;
    double RTTct = 1.0f; //RTT change trend

    RTTct = (_avgRTTNear - _avgRTTFar) / 1000.0f;
    alpha = alpha + cTa2 * RTTct + cTa3 / sqrt(_avgRTTNear + 2.0);

    //alpha = BWE_MIN(alpha, 1.050);
    //alpha = BWE_MAX(alpha, 1.001);

    alpha = pow(alpha, static_cast<double>(reactionTimeMs  + 0.5f)/ 1000);
    alpha = BWE_MIN(alpha, 1.5);

    if (_rcRegion == kRcNearMax)
    {
        // We're close to our previous maximum. Try to stabilize the
        // bit rate in this region, by increasing in smaller steps.
        alpha = 1.0 + (alpha - 1.0) / 3.0;
    }
    //else if (_rcRegion == kRcMaxUnknown)
    //{
    //    alpha = 1.0 + (alpha - 1.0) * 1.25;
    //}

    const double theta1 = -0.4346f;
    const double theta2 = 1.4346f;
    //first pass: smooth the bitrate by long time overuse statistic
    double ltOveruseTrace = 1.0f;
    if (currentBitRate > _meanOverUseBr * 0.80 && _meanOverUseBr != 0)
    {
        double ov = _overUseRate * 10000;
        ov = BWE_MAX(1, ov);
        ltOveruseTrace = log10(ov) * theta1 + theta2;
        ltOveruseTrace = BWE_MAX(0.0f, ltOveruseTrace);
        ltOveruseTrace = BWE_MIN(1.0f, ltOveruseTrace);
        alpha = 1.0 + (alpha - 1.0) * ltOveruseTrace;
    }

    //second pass: smooth the bitrate by long long time overuse statistic
    double lltOveruseTrace = 1.0f;
    if ((currentBitRate > _meanOverUseBr && _meanOverUseBr != 0) ||
        currentBitRate > _asufficBitRate)
    {
        double llov = _lloverUseRate * 10000;
        llov = BWE_MAX(1, llov);
        lltOveruseTrace = log10(llov) * theta1 + theta2;
        lltOveruseTrace = BWE_MAX(0.0f, lltOveruseTrace);
        lltOveruseTrace = BWE_MIN(1.0f, lltOveruseTrace);
        alpha = 1.0 + (alpha - 1.0) * lltOveruseTrace;
    }

    ////trace to slowdown or hold for incomming bitrate catch up
    //double incomTraceRate = 0;
    ////linear function: y = 2x - 1.0
    //incomTraceRate  = 2.0 * incommingRatio - 1.0;
    //incomTraceRate = BWE_MAX(incomTraceRate, 0);
    //incomTraceRate = BWE_MIN(incomTraceRate, 1.0);
    //alpha = 1.0 + (alpha - 1.0) * incomTraceRate;

    return alpha;
}

void RemoteRateControl::UpdateOveruseRate(WebRtc_UWord32 incomingBitRate, RateControlState curState)
{
    double overUseRate = _overUseRate;
    double lloverUseRate = _lloverUseRate;
    const float alpha = 0.03f;
    WebRtc_UWord32 meanOverUseBr = _meanOverUseBr;
    double p = 0.0f;

    if (curState == kRcDecrease)
    {
        p = 1.0f;
        if (meanOverUseBr != 0)
        {
            meanOverUseBr = incomingBitRate * alpha + meanOverUseBr * (1-alpha);
        }
        else
        {
            meanOverUseBr = incomingBitRate;
        }
    }
    else
    {
        p = 0.0f;
    }

    //the overuse rate function:
    //                                R[n] - p
    // R[n+1] = R[n]  -  ------------
    //                                   W
    overUseRate = overUseRate - (overUseRate - p)/kOveruseMeasureWindow;
    lloverUseRate = lloverUseRate - (lloverUseRate - p)/(kOveruseMeasureWindow * 10);

    _overUseRate = overUseRate;
    _lloverUseRate = lloverUseRate;
    _meanOverUseBr = meanOverUseBr;
}

void RemoteRateControl::UpdateRTT(WebRtc_UWord32 RTT)
{
    const float farWeight = 0.90f;
    const float nearWeight = 0.75f;

    if (0 == _avgRTTFar)
    {
        _avgRTTFar = RTT;
    }
    else
    {
        _avgRTTFar = _avgRTTFar * farWeight + RTT * (1.0 - farWeight);
    }

    if (0 == _avgRTTNear)
    {
        _avgRTTNear = RTT;
    }
    else
    {
        _avgRTTNear = _avgRTTNear * nearWeight + RTT * (1.0 - nearWeight);
    }

}

void RemoteRateControl::UpdateMaxBitRateEstimate(float incomingBitRateKbps)
{
    const float alpha = 0.05f;
    if (_avgMaxBitRateKbps == -1.0f)
    {
        _avgMaxBitRateKbps = incomingBitRateKbps;
    }
    else
    {
        _avgMaxBitRateKbps = (1 - alpha) * _avgMaxBitRateKbps +
            alpha * incomingBitRateKbps;
    }
    // Estimate the max bit rate variance and normalize the variance
    // with the average max bit rate.
    const float norm = BWE_MAX(_avgMaxBitRateKbps, 1.0f);
    _varMaxBitRateKbps = (1 - alpha) * _varMaxBitRateKbps +
        alpha * (_avgMaxBitRateKbps - incomingBitRateKbps) *
        (_avgMaxBitRateKbps - incomingBitRateKbps) /
        norm;
    // 0.4 ~= 14 kbit/s at 500 kbit/s
    if (_varMaxBitRateKbps < 0.4f)
    {
        _varMaxBitRateKbps = 0.4f;
    }
    // 2.5f ~= 35 kbit/s at 500 kbit/s
    if (_varMaxBitRateKbps > 2.5f)
    {
        _varMaxBitRateKbps = 2.5f;
    }
}

void RemoteRateControl::ChangeState(const RateControlInput& input, WebRtc_Word64 nowMs)
{
    switch (_currentInput._bwState)
    {
    case kBwNormal:
        {
            if (_rcState == kRcHold)
            {
                //_lastBitRateChange = nowMs;
                ChangeState(kRcIncrease);
                if (_initializedBitRate)
                {
                    ChangeRegion(kRcNearMax);
                }
            }
            else if (_rcState==kRcDecrease)
            {
                ChangeState(kRcHold);
            }
            break;
        }
    case kBwOverusing:
        {
            if (_rcState != kRcDecrease)
            {
                ChangeState(kRcDecrease);
            }
            break;
        }
    case kBwUnderUsing:
        {
            ChangeState(kRcHold);
            break;
        }
    }
}

void RemoteRateControl::ChangeRegion(RateControlRegion region)
{
    _rcRegion = region;
    switch (_rcRegion)
    {
    case kRcAboveMax:
    case kRcMaxUnknown:
        {
            _beta = 0.85f;
            break;
        }
    case kRcNearMax:
        {
            _beta = 0.90f;
            break;
        }
    }
}

void RemoteRateControl::ChangeState(RateControlState newState)
{
    _cameFromState = _rcState;
    _rcState = newState;
    char state1[15];
    char state2[15];
    char state3[15];
    StateStr(_cameFromState, state1);
    StateStr(_rcState, state2);
    StateStr(_currentInput._bwState, state3);
}

void RemoteRateControl::StateStr(RateControlState state, char* str)
{
    switch (state)
    {
    case kRcDecrease:
        strncpy(str, "DECREASE", 9);
        break;
    case kRcHold:
        strncpy(str, "HOLD", 5);
        break;
    case kRcIncrease:
        strncpy(str, "INCREASE", 9);
        break;
    }
}

void RemoteRateControl::StateStr(BandwidthUsage state, char* str)
{
    switch (state)
    {
    case kBwNormal:
        strncpy(str, "NORMAL", 7);
        break;
    case kBwOverusing:
        strncpy(str, "OVER USING", 11);
        break;
    case kBwUnderUsing:
        strncpy(str, "UNDER USING", 12);
        break;
    }
}

void RemoteRateControl::UpdateLossMetric(WebRtc_UWord32 loss,
                                         WebRtc_UWord32 accumLoss,WebRtc_UWord32 maxSeq, WebRtc_UWord32 nowMs)
{
    //init
    if(_numUpdate == 0)
    {
        _prevAccumLoss = accumLoss;
        _prevTime = nowMs;
        _prevMaxSeqNo = maxSeq;
        _numUpdate = 1;
        return;
    }
    else if (_numUpdate == 1)
    {

        if (maxSeq-_prevMaxSeqNo>=kUpdateLossPacketInterval
            || (nowMs-_prevTime>=kMinTimeInterval
            && maxSeq-_prevMaxSeqNo>=kMinPacketInterval))
        {
            _avgLoss=255*(accumLoss-_prevAccumLoss)/BWE_MAX((maxSeq-_prevMaxSeqNo),1);

            _prevAccumLoss = accumLoss;
            _prevTime = nowMs;
            _prevMaxSeqNo = maxSeq;
            _numUpdate = 2;
            if (_avgLoss > kLossOveruseHardlimit)
            {
                _lossHypothesis = kBwOverusing;
            }
        }
        return;
    }

    WebRtc_UWord32 packetCount = 0;
    if (maxSeq > _prevMaxSeqNo)
        packetCount = maxSeq - _prevMaxSeqNo;
    WebRtc_UWord32 timeCount = nowMs-_prevTime;
    WebRtc_UWord32 lossCount = 0;
    if (accumLoss > _prevAccumLoss)
        lossCount = accumLoss-_prevAccumLoss;
    WebRtc_UWord32 tempLoss = 0;
    if (packetCount)
        tempLoss = 255 * lossCount / packetCount;
    if (lossCount > packetCount)
        tempLoss = 255;

    if ((packetCount>=kUpdateLossPacketInterval)||
        (packetCount>=kMinPacketInterval && timeCount>=kMinTimeInterval))
    {//update
        assert(tempLoss>=0 && tempLoss <=255);
        WebRtc_UWord32 packetRate = BWE_MIN(300,1000*packetCount/BWE_MAX(timeCount,100));

        if (_numUpdate>=5)
        {//detect
            WebRtc_UWord32 diffLoss=tempLoss-_avgLoss;
            if (tempLoss == 0)
            {//when loss is 0, it means buffer is not full enough to cause drop of packet, so it's safe to assume it's normal now
                //skip underusing stage from overusing to underusing to normal
                _lossHypothesis = kBwNormal;
                _overusePacketCount = 0;
                _normalCount = 0;
            }
            else
            {
                float beta = 0.9f;

                _overusePacketCount = _overusePacketCount*beta+diffLoss*packetCount;
                float ratio = sqrt(60.0f/BWE_MAX(packetRate,30.0f))*log(BWE_MAX(_avgLoss,2.7183f))/BWE_MAX(_avgLoss,2.7183f);
                if (tempLoss<=kLossOveruseSoftlimit)
                {
                    _lossHypothesis = kBwNormal;
                    _normalCount++;
                }
                else if (_overusePacketCount>kPacketThreshold/ratio || tempLoss > kLossOveruseHardlimit)
                {
                    _lossHypothesis = kBwOverusing;
                    _normalCount = 0;
                }
                else if (_overusePacketCount<-kPacketThreshold/ratio)
                {
                    _lossHypothesis = kBwUnderUsing;
                    _normalCount = 0;
                }
                else
                {
                    _lossHypothesis = kBwNormal;
                    _normalCount++;
                }
            }
        }

        float alpha = 1.0f/16;
        if (_numUpdate<5 || _lossHypothesis == kBwUnderUsing || tempLoss == 0)
        {//fast adapt at init stage or underuse stage
            alpha = 1.0f/8;
        }
        float beta = pow(1-alpha,(float)(timeCount)/kMinTimeInterval);
        _avgLoss = beta*_avgLoss+(1-beta)*tempLoss;;
        _varLoss = beta * _varLoss + (1-beta) * (_avgLoss - tempLoss)*(_avgLoss - tempLoss) ;

        _prevAccumLoss = accumLoss;
        _prevTime = nowMs;
        _prevMaxSeqNo = maxSeq;
        ++_numUpdate;
        _numUpdate = BWE_MIN(_numUpdate,5);
        if (_normalCount>=5 && tempLoss<_avgLoss+sqrt((double)_varLoss))
        {
            _baseLoss = _avgLoss;
        }
    }
    else if (timeCount>=kUpdateLossTimeInterval && _lossHypothesis != kBwNormal
        && tempLoss<=BWE_MAX(_baseLoss,kLossOveruseSoftlimit))
    {// this happens when Bitrate is extremely low, i.e. packet rate below 10p/s
        //can't wait for enough packets to calculate accurate loss rate; have a peek at current rough loss rate
        // reset to normal if ideal
        _lossHypothesis = kBwNormal;
        _overusePacketCount = 0;
        _normalCount = 0;
    }

    return;
}

void
RemoteRateControl::SetAsufficBitRate(WebRtc_UWord32 asufficBitRate)
{
    asufficBitRate = BWE_MAX(160000, asufficBitRate);  //assume min is CIF 10hz
    asufficBitRate = BWE_MIN(2550000, asufficBitRate); //assume max is 720p 30hz
    _asufficBitRate = asufficBitRate;
}

void RemoteRateControl::SetCpuLimitedRemoteBR(float percent,WebRtc_UWord32 minBrKbit,WebRtc_UWord32 maxBrKbit)
{
    //cpu load ctrl is not be used, remove bitrate limit function.
    return;

    if (percent>0 && percent<1)
        _bitratePercent = percent;

    if (minBrKbit>0)
        _minCpuLimitedBr = minBrKbit*1000;

    WebRtc_UWord32 maxBr = maxBrKbit*1000;
    if (maxBr>_minConfiguredBitRate && maxBr<_maxConfiguredBitRate)
        _maxConfiguredBitRate = maxBr;
}

/*
*  Simple bandwidth estimation. Depends a lot on bwEstimateIncoming and packetLoss.
*/
// protected
WebRtc_UWord32 SendRateControl::ShapeSimple(WebRtc_Word32 packetLoss,
                                                WebRtc_Word32 rtt,
                                                WebRtc_UWord32 sentBitrate,
                                                WebRtc_Word64 nowMS)
{
    WebRtc_UWord32 newBitRate = _bitRate;
    WebRtc_UWord32 newBitRateByRtt = 0;
    const WebRtc_UWord32 increaseRateLossThresHold = 0.005*(_avgLoss*_avgLoss)-0.35*_avgLoss+15.0;
    const WebRtc_UWord32 holdRateLossThresHold = increaseRateLossThresHold;
    const float increaseRatio = 1.1f;
    bool bTimeAccumulateLoss = false;
    
    float timeInterval = nowMS - _timeLastChange;
    float alpha;

    
    if (timeInterval < 10)
    {
        timeInterval = 10;
    }
    else if (timeInterval > 2000) 
    {
        timeInterval = 2000;
    }

    _timeLastChange = nowMS;

    if(_timeAccumulateLoss < 20000.0f)
    {
        _timeAccumulateLoss += timeInterval;
        alpha = pow(0.5f, timeInterval/2000.0f);
        _avgLoss = alpha * _avgLoss + (1.0f - alpha) * packetLoss;
    }
    else
    {
        alpha = pow(0.5f, timeInterval/30000.0f);
        _avgLoss = alpha * _avgLoss + (1.0f - alpha) * packetLoss;
        bTimeAccumulateLoss = true;
    }

    // Estimate the bandwidth by rtt
    newBitRateByRtt = ShapeSimpleRtt(rtt, sentBitrate, timeInterval);

    
    if (packetLoss > _avgLoss + holdRateLossThresHold && bTimeAccumulateLoss)
    {
        double decreaseLoss = packetLoss - _avgLoss;
        alpha = static_cast<float>(256.0f - decreaseLoss) / 256.0f;
        alpha = pow(alpha, timeInterval/2000.0f);
        if(alpha < 0.5f)
            alpha = 0.5f;
        newBitRate = static_cast<WebRtc_UWord32>(_bitRate * alpha);
    }
    else
    {
        // increase rate by increaseRatio per second
        alpha = pow(increaseRatio, timeInterval/1000.0f);
        newBitRate = static_cast<WebRtc_UWord32>(_bitRate * alpha);

        // add 3 kbps extra, just to make sure that we do not get stuck
        // (gives a little extra increase at low rates, negligible at higher rates)
        newBitRate += 3000;
    }
    
    
    if (newBitRateByRtt > 0 && newBitRate > newBitRateByRtt)
    {
        newBitRate = newBitRateByRtt;
    }

    // Calculate what rate TFRC would apply in this situation
    WebRtc_Word32 tfrcRate = CalcTFRCbps(1000, rtt, packetLoss); // scale loss to Q0 (back to [0, 255])

    if (tfrcRate > 0 &&
        static_cast<WebRtc_UWord32>(tfrcRate) > newBitRate)
    {
        // do not reduce further if rate is below TFRC rate
        //fix vcm setrate huge jump bug
        if (static_cast<WebRtc_UWord32>(tfrcRate) > newBitRate * 1.2f) {
            newBitRate *= 1.2f;
        }
        else {
            newBitRate = tfrcRate;
        }
    }

    return newBitRate;
}

/* Calculate the rate that TCP-Friendly Rate Control (TFRC) would apply.
 * The formula in RFC 3448, Section 3.1, is used.
 */

// protected
WebRtc_Word32 SendRateControl::CalcTFRCbps(WebRtc_Word16 avgPackSizeBytes,
                                               WebRtc_Word32 rttMs,
                                               WebRtc_Word32 packetLoss)
{
    if (avgPackSizeBytes <= 0 || rttMs <= 0 || packetLoss <= 0)
    {
        // input variables out of range; return -1
        return -1;
    }

    double R = static_cast<double>(rttMs)/1000; // RTT in seconds
    int b = 1; // number of packets acknowledged by a single TCP acknowledgement; recommended = 1
    double t_RTO = 4.0 * R; // TCP retransmission timeout value in seconds; recommended = 4*R
    double p = static_cast<double>(packetLoss)/255; // packet loss rate in [0, 1)
    double s = static_cast<double>(avgPackSizeBytes);

    // calculate send rate in bytes/second
    double X = s / (R * sqrt(2 * b * p / 3) + (t_RTO * (3 * sqrt( 3 * b * p / 8) * p * (1 + 32 * p * p))));

    return (static_cast<WebRtc_Word32>(X*8)); // bits/second
}

/*
*  Work with ShapeSimple, estimate the bandwidth by rtt
*/
// protected
WebRtc_UWord32 SendRateControl::ShapeSimpleRtt(WebRtc_Word32 rtt,
                                                WebRtc_UWord32 sentBitrate, float timeInterval)
{
    WebRtc_UWord32 newBitRate = 0;   
    float alpha;
    
    if (rtt > 3000)
    {  
        alpha = 1000.0f/(float)rtt;
        alpha = pow(alpha, timeInterval/1000.0f);
        alpha = BWE_MAX(alpha, 0.5f);
        newBitRate = static_cast<WebRtc_UWord32>(sentBitrate * alpha);
    }
    else
    {  // increase rate by 8% per second
        alpha = pow(1.08f, timeInterval/1000.0f);
        newBitRate = static_cast<WebRtc_UWord32>(_bitRate * alpha);
        newBitRate += 2000;
    }

    return newBitRate;
}


} //end namespace BWE
