#ifdef __cplusplus
extern "C" {
#endif 
struct ebur128 {
	float integrated;
	float integ_thr;
	float range_thr;
	float range_min;
	float range_max;
	float maxloudn_M;
	float maxloudn_S;
	float range; // redundant -- (range_max-range_min)
};

int ebur128analyse (const char *, struct ebur128 *);

#ifdef __cplusplus
}
#endif 
