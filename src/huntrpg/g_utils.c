
#include "g_local.h"

float vectoyaw(vec3_t vec)
{
	float yaw;

	// FIXES HERE FROM 3.20 -FB
	if ( /*vec[YAW] == 0 && */ vec[PITCH] == 0)
	{
		yaw = 0;
		if (vec[YAW] > 0)
			yaw = 90;
		else if (vec[YAW] < 0)
			yaw = -90;
	}
	// ^^^
	else
	{
		yaw = (int)(atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	return yaw;
}

// Reki: Yoinked from Quake 3 mathlib.c
void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3])
{
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
		in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
		in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
		in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
		in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
		in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
		in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
		in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
		in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
		in1[2][2] * in2[2][2];
}

// Reki: Yoinked vectoangles2 from darkplaces
void vectoangles2(vec3_t angles, const vec3_t forward, const vec3_t up, qboolean flippitch)
{
	if (forward[0] == 0 && forward[1] == 0)
	{
		if (forward[2] > 0)
		{
			angles[PITCH] = -M_PI * 0.5;
			angles[YAW] = up ? atan2(-up[1], -up[0]) : 0;
		}
		else
		{
			angles[PITCH] = M_PI * 0.5;
			angles[YAW] = up ? atan2(up[1], up[0]) : 0;
		}
		angles[ROLL] = 0;
	}
	else
	{
		angles[YAW] = atan2(forward[1], forward[0]);
		angles[PITCH] = -atan2(forward[2], sqrt(forward[0] * forward[0] + forward[1] * forward[1]));
		// note: we know that angles[PITCH] is in ]-pi/2..pi/2[ due to atan2(anything, positive)
		if (up)
		{
			vec_t cp = cos(angles[PITCH]), sp = sin(angles[PITCH]);
			// note: we know cp > 0, due to the range angles[pitch] is in
			vec_t cy = cos(angles[YAW]), sy = sin(angles[YAW]);
			vec3_t tleft, tup;
			tleft[0] = -sy;
			tleft[1] = cy;
			tleft[2] = 0;
			tup[0] = sp * cy;
			tup[1] = sp * sy;
			tup[2] = cp;
			angles[ROLL] = -atan2(DotProduct(up, tleft), DotProduct(up, tup));
			// for up == '0 0 1', this is
			// angles[ROLL] = -atan2(0, cp);
			// which is 0
		}
		else
			angles[ROLL] = 0;

		// so no up vector is equivalent to '1 0 0'!
	}

	// now convert radians to degrees, and make all values positive
	VectorScale(angles, 180.0 / M_PI, angles);
	if (flippitch)
		angles[PITCH] *= -1;
	if (angles[PITCH] < 0) angles[PITCH] += 360;
	if (angles[YAW] < 0) angles[YAW] += 360;
	if (angles[ROLL] < 0) angles[ROLL] += 360;
}

void vectoangles(vec3_t value1, vec3_t angles)
{
	float forward;
	float yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else if (value1[2] < 0)
			pitch = 270;
		else
			pitch = 180;
	}
	else
	{
		// FIXES HERE FROM 3.20  -FB
		// zucc changing casts to floats
		if (value1[0])
			yaw = (float)(atan2(value1[1], value1[0]) * 180 / M_PI);
		else if (value1[1] > 0)
			yaw = 90;
		else
			yaw = -90;
		// ^^^
		if (yaw < 0)
			yaw += 360;

		forward = sqrtf(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch = (float)(atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	angles[PITCH] = 360 - pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

int anglediff(int x, int y)
{
	int a;
	a = x - y;
	a = (a + 180) % 360 - 180;
	if (a > 180) a -= 360;
	if (a < -180) a += 360;

	return a;
}