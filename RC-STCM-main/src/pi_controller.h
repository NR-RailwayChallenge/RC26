#ifndef PI_CONTROLLER_H
#define PI_CONTROLLER_H

class PiController{

private:
    float _ts;
    float _kp;
    float _ki;
    float _previousIk;
    float _previousError;

public:
    PiController(float ts, float kp, float ki){
        _ts = ts;
        _kp = kp;
        _ki = ki;
    }

    float GetNextControlOutput(float error){
        float ik = _previousIk + (_ts*((error-_previousError)/2.0f));
        _previousError = error;
        _previousIk = ik;
        return (_kp*error) + (_ki * ik);
    }

    void Reset(){
        _previousError = 0;
        _previousIk = 0;
    }

};

#endif