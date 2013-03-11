#ifndef _SECP256K1_ECDSA_
#define _SECP256K1_ECDSA_

namespace secp256k1 {

bool ParsePubKey(GroupElemJac &elem, const unsigned char *pub, int size) {
    if (size == 33 && (pub[0] == 0x02 || pub[0] == 0x03)) {
        FieldElem x;
        x.SetBytes(pub+1);
        elem.SetCompressed(x, pub[0] == 0x03);
    } else if (size == 65 && (pub[0] == 0x04 || pub[0] == 0x06 || pub[0] == 0x07)) {
        FieldElem x,y;
        x.SetBytes(pub+1);
        y.SetBytes(pub+33);
        elem = GroupElem(x,y);
        if ((pub[0] == 0x06 || pub[0] == 0x07) && y.IsOdd() != (pub[0] == 0x07))
            return false;
    } else {
        return false;
    }
    return elem.IsValid();
}

class Signature {
private:
    Number r,s;

public:
    Signature(Context &ctx) : r(ctx), s(ctx) {}

    bool Parse(const unsigned char *sig, int size) {
        if (sig[0] != 0x30) return false;
        if (sig[1] != size-2) return false;
        int lenr = sig[3];
        if (4+lenr >= size) return false;
        int lens = sig[lenr+5];
        if (lenr+lens+6 != size) return false;
        if (sig[2] != 0x02) return false;
        if (lenr == 0) return false;
        if (sig[lenr+4] != 0x02) return false;
        if (lens == 0) return false;
        r.SetBytes(sig+4, lenr);
        s.SetBytes(sig+6+lenr, lens);
        return true;
    }

    bool RecomputeR(Context &ctx, Number &r2, const GroupElemJac &pubkey, const Number &message) {
        const GroupConstants &c = GetGroupConst();

        if (r.IsNeg() || s.IsNeg())
            return false;
        if (r.IsZero() || s.IsZero())
            return false;
        if (r.Compare(c.order) >= 0 || s.Compare(c.order) >= 0)
            return false;

        Context ct(ctx);
        Number sn(ct), u1(ct), u2(ct);
        sn.SetModInverse(ct, s, c.order);
        u1.SetModMul(ct, sn, message, c.order);
        u2.SetModMul(ct, sn, r, c.order);
        GroupElemJac pr; ECMult(ct, pr, pubkey, u2, u1);
        if (pr.IsInfinity())
            return false;
        FieldElem xr; pr.GetX(ct, xr);
        unsigned char xrb[32]; xr.GetBytes(xrb);
        r2.SetBytes(xrb,32); r2.SetMod(ct,r2,c.order);
        return true;
    }

    bool Verify(Context &ctx, const GroupElemJac &pubkey, const Number &message) {
        Context ct(ctx);
        Number r2(ct);
        if (!RecomputeR(ct, r2, pubkey, message))
            return false;
        return r2.Compare(r) == 0;
    }

    void SetRS(const Number &rin, const Number &sin) {
        r = rin;
        s = sin;
    }
};

int VerifyECDSA(const unsigned char *msg, int msglen, const unsigned char *sig, int siglen, const unsigned char *pubkey, int pubkeylen) {
    Context ctx;
    Number m(ctx); 
    Signature s(ctx);
    GroupElemJac q;
    m.SetBytes(msg, msglen);
    if (!ParsePubKey(q, pubkey, pubkeylen))
        return -1;
    if (!s.Parse(sig, siglen))
        return -2;
    if (!s.Verify(ctx, q, m))
        return 0;
    return 1;
}

}

#endif
