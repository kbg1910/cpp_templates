// Be careful about overflow as we are using integer everywhere
// modular FFT - kbg

template<const int &MOD>
struct _m_int  // This is our new type which will do operations under modulo MOD
{
	int val;   // Value of variable

	_m_int(int64_t v = 0)  // Typecasting into our data type from 64 bit integer
	{
		if (v < 0) v = v % MOD + MOD;
		if (v >= MOD) v %= MOD;
		val = v;
	}

	static int mod_inv(int a, int m = MOD)
	{
		// https://en.wikipedia.org/wiki/Extended_Euclidean_algorithm#Example
		int g = m, r = a, x = 0, y = 1;

		while (r != 0)
		{
			int q = g / r;
			g %= r; swap(g, r);
			x -= q * y; swap(x, y);
		}

		return x < 0 ? x + m : x;
	}

	explicit operator int() const
	{
		return val;
	}

	explicit operator int64_t() const
	{
		return val;
	}

	_m_int & operator += (const _m_int & other) // Addition
	{
		val -= MOD - other.val;
		if (val < 0) val += MOD;
		return *this;
	}

	_m_int & operator -= (const _m_int & other) // Subtraction
	{
		val -= other.val;
		if (val < 0) val += MOD;
		return *this;
	}

	static unsigned fast_mod(uint64_t x, unsigned m = MOD) // Mod operation
	{
#if !defined(_WIN32) || defined(_WIN64)
		return x % m;
#endif
		// Optimized mod for Codeforces 32-bit machines.
		// x must be less than 2^32 * m for this to work, so that x / m fits in a 32-bit integer.
		unsigned x_high = x >> 32, x_low = (unsigned) x;
		unsigned quot, rem;
		asm("divl %4\n"
		: "=a" (quot), "=d" (rem)
		: "d" (x_high), "a" (x_low), "r" (m));
		return rem;
	}

	_m_int & operator*=(const _m_int & other) // Multiplication
	{
		val = fast_mod((uint64_t) val * other.val);
		return *this;
	}

	_m_int& operator/=(const _m_int & other) // Division
	{
		return *this *= other.inv();
	}

	friend _m_int operator+(const _m_int & a, const _m_int & b) { return _m_int(a) += b; }
	friend _m_int operator-(const _m_int & a, const _m_int & b) { return _m_int(a) -= b; }
	friend _m_int operator*(const _m_int & a, const _m_int & b) { return _m_int(a) *= b; }
	friend _m_int operator/(const _m_int & a, const _m_int & b) { return _m_int(a) /= b; }

	_m_int & operator++() // Pre-increment
	{
		val = val == MOD - 1 ? 0 : val + 1;
		return *this;
	}

	_m_int & operator--() // Pre-decrement
	{
		val = val == 0 ? MOD - 1 : val - 1;
		return *this;
	}

	_m_int operator++(int) { _m_int before = *this; ++*this; return before; }  // Post increment
	_m_int operator--(int) { _m_int before = *this; --*this; return before; }  // Post decrement

	_m_int operator-() const  // Change sign
	{
		return val == 0 ? 0 : MOD - val;
	}

	// Boolean operators
	bool operator==(const _m_int & other) const { return val == other.val; }
	bool operator!=(const _m_int & other) const { return val != other.val; }
	bool operator< (const _m_int & other) const { return val <  other.val; }
	bool operator<=(const _m_int & other) const { return val <= other.val; }
	bool operator> (const _m_int & other) const { return val >  other.val; }
	bool operator>=(const _m_int & other) const { return val >= other.val; }

	_m_int inv() const  // Calculating inverse
	{
		return mod_inv(val);
	}

	_m_int pow(int64_t p) const  // Calculating power
	{
		if (p < 0)
			return inv().pow(-p);

		_m_int a = *this, result = 1;

		while (p > 0)
		{
			if (p & 1)
			{
				result *= a;
			}
			a *= a;
			p >>= 1;
		}

		return result;
	}

	friend ostream & operator<<(ostream & os, const _m_int & m) // Writing output
	{
		return os << m.val;
	}

};

extern const int MOD = 998244353;
using mod_int = _m_int<MOD>;


template<const int &MOD>
struct NTT
{
	using ntt_int = _m_int<MOD>;

	vector<ntt_int> roots = {0, 1};
	vector<int> bit_reverse;
	int max_size = -1;
	ntt_int root;

	void reset()
	{
		roots = {0, 1};
		max_size = -1;
	}

	static bool is_power_of_two(int n) {  return (n & (n - 1)) == 0;  }

	static int round_up_power_two(int n)
	{
		while (n & (n - 1))
			n = (n | (n - 1)) + 1;

		return max(n, 1);
	}

	// Given n (a power of two), finds k such that n == 1 << k.
	static int get_length(int n)
	{
		assert(is_power_of_two(n));
		return __builtin_ctz(n);
	}

	// Rearranges the indices to be sorted by lowest bit first, then second lowest, etc., rather than highest bit first.
	// This makes even-odd div-conquer much easier.
	void bit_reorder(int n, vector<ntt_int> &values)
	{
		if ((int) bit_reverse.size() != n)
		{
			bit_reverse.assign(n, 0);
			int length = get_length(n);

			for (int i = 1; i < n; i++)
				bit_reverse[i] = (bit_reverse[i >> 1] >> 1) | ((i & 1) << (length - 1));
		}

		for (int i = 0; i < n; i++)
		{
			if (i < bit_reverse[i])
			{
				swap(values[i], values[bit_reverse[i]]);
			}
		}
	}

	void find_root()
	{
		max_size = 1 << __builtin_ctz(MOD - 1);
		root = 2;

		// Find a max_size-th primitive root of MOD.
		while (!(root.pow(max_size) == 1 && root.pow(max_size / 2) != 1))
		{
			root++;
		}
	}

	void prepare_roots(int n)
	{
		if (max_size < 0)
			find_root();

		assert(n <= max_size);

		if ((int) roots.size() >= n)
			return;

		int length = get_length(roots.size());
		roots.resize(n);

		// The roots array is set up such that for a given power of two n >= 2, roots[n / 2] through roots[n - 1] are
		// the first half of the n-th primitive roots of MOD.
		while (1 << length < n)
		{
			// z is a 2^(length + 1)-th primitive root of MOD.
			ntt_int z = root.pow(max_size >> (length + 1));

			for (int i = 1 << (length - 1); i < 1 << length; i++)
			{
				roots[2 * i] = roots[i];
				roots[2 * i + 1] = roots[i] * z;
			}

			length++;
		}
	}

	void fft_iterative(int N, vector<ntt_int> &values)
	{
		assert(is_power_of_two(N));
		prepare_roots(N);
		bit_reorder(N, values);

		for (int n = 1; n < N; n *= 2)
		{
			for (int start = 0; start < N; start += 2 * n)
			{
				for (int i = 0; i < n; i++)
				{
					ntt_int even = values[start + i];
					ntt_int odd = values[start + n + i] * roots[n + i];
					values[start + n + i] = even - odd;
					values[start + i] = even + odd;
				}
			}
		}
	}

	void invert_fft(int N, vector<ntt_int> &values)
	{
		ntt_int inv_N = ntt_int(N).inv();

		for (int i = 0; i < N; i++)
		{
			values[i] *= inv_N;
		}

		reverse(values.begin() + 1, values.end());
		fft_iterative(N, values);
	}

	const int FFT_CUTOFF = 150;

	// Note: `circular = true` can be used for a 2x speedup when only the `max(n, m) - min(n, m) + 1` fully overlapping
	// ranges are needed. It computes results using indices modulo the power-of-two FFT size; see the brute force below.
	template<typename T>
	vector<T> mod_multiply(const vector<T> &_left, const vector<T> &_right, bool circular = false)
	{
		if (_left.empty() || _right.empty())
			return {};

		vector<ntt_int> left(_left.begin(), _left.end());
		vector<ntt_int> right(_right.begin(), _right.end());

		int n = left.size();
		int m = right.size();

		int output_size = circular ? round_up_power_two(max(n, m)) : n + m - 1;

		// Brute force when either n or m is small enough.
		if (min(n, m) < FFT_CUTOFF)
		{
			auto &&mod_output_size = [&](int x)
			{
				return x < output_size ? x : x - output_size;
			};

			static const uint64_t U64_BOUND = numeric_limits<uint64_t>::max() - (uint64_t) MOD * MOD;
			vector<uint64_t> result(output_size, 0);

			for (int i = 0; i < n; i++)
			{
				for (int j = 0; j < m; j++)
				{
					int index = mod_output_size(i + j);
					result[index] += (uint64_t) (int64_t) left[i] * (int64_t) right[j];

					if (result[index] > U64_BOUND)
						result[index] %= MOD;
				}
			}
			for (uint64_t &x : result)
				if (x >= MOD)
					x %= MOD;

			return vector<T>(result.begin(), result.end());
		}

		int N = round_up_power_two(output_size);
		left.resize(N, 0);
		right.resize(N, 0);

		if (left == right)
		{
			fft_iterative(N, left);
			right = left;
		}
		else
		{
			fft_iterative(N, left);
			fft_iterative(N, right);
		}

		for (int i = 0; i < N; i++)
		{
			left[i] *= right[i];
		}

		invert_fft(N, left);
		return vector<T>(left.begin(), left.begin() + output_size);
	}

	template<typename T>
	vector<T> mod_power(const vector<T> &v, int exponent)
	{
		assert(exponent >= 0);
		vector<T> result = {1};

		if (exponent == 0)
			return result;

		for (int k = 31 - __builtin_clz(exponent); k >= 0; k--)
		{
			result = mod_multiply(result, result);

			if (exponent >> k & 1)
				result = mod_multiply(result, v);
		}

		return result;
	}

	template<typename T>
	vector<T> mod_multiply_all(const vector<vector<T>> &polynomials)
	{
		if (polynomials.empty())
			return {1};

		struct compare_size
		{
			bool operator()(const vector<T> &x, const vector<T> &y)
			{
				return x.size() > y.size();
			}
		};

		priority_queue<vector<T>, vector<vector<T>>, compare_size> pq(polynomials.begin(), polynomials.end());

		while (pq.size() > 1)
		{
			vector<T> a = pq.top(); pq.pop();
			vector<T> b = pq.top(); pq.pop();
			pq.push(mod_multiply(a, b));
		}

		return pq.top();
	}
};
