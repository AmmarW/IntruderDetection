//Sharp IR sensor library file

#pragma once

#include <Arduino.h>

class IntruderDetection
	{
		public:

			using sensorCode = const uint8_t ;

			IntruderDetection( sensorCode _sensorType , uint8_t _sensorPin ) : sensorType( _sensorType ) , pin( _sensorPin ) {}

			uint8_t getDistance( bool avoidBurstRead = true ) ;

			static sensorCode GP2Y0A41SK0F = 0 ;
			static sensorCode GP2Y0A21YK0F = 1 ;
			static sensorCode GP2Y0A02YK0F = 3 ;

		protected:

			uint8_t sensorType, pin ;

		private:

			uint32_t lastTime = 0 ;
	};
