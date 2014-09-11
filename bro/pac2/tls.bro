module TLS;

export {
  type SignatureAndHashAlgorithm: record {
    hash: HashAlgorithm;
    signature: SignatureAlgorithm;
  };
}
