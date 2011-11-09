const char *get_qaac_version()
{
#ifdef REFALAC
    return "0.08";
#else
    return "1.00";
#endif
}
