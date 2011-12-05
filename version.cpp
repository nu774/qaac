const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.19";
#else
    return "1.08";
#endif
}
