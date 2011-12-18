const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.24";
#else
    return "1.13";
#endif
}
