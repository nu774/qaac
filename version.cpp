const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.32";
#else
    return "1.21";
#endif
}
