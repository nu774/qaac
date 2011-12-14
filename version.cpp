const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.21";
#else
    return "1.10";
#endif
}
